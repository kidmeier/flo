#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "core.log.h"
#include "res.core.h"
#include "sync.once.h"
#include "sys.fs.h"

// Public API /////////////////////////////////////////////////////////////////

struct Importer {

	char  typeid[4];
	char *ext;

	import_Resource_f import;

	struct Importer *next;

};

static struct Res_Type *res_types = NULL;
static struct Importer *importers = NULL;

void  register_Res_importer( const char typeid[4],
                             const char* ext, 
                             import_Resource_f impfunc ) {

	struct Importer* ldr = malloc( sizeof(struct Importer) 
	                               + strlen(ext)+1 );

	memcpy( ldr->typeid, typeid, sizeof(typeid) );

	ldr->ext = (pointer)ldr + sizeof(struct Importer);
	strcpy( ldr->ext, ext );
 
	ldr->import = impfunc;

	// insert into list
	ldr->next = importers;
	importers = ldr;

}

void  register_Res_type( const char id[4],
                         write_Resource_f writefunc,
                         read_Resource_f readfunc ) {

	struct Res_Type* restype = malloc( sizeof(struct Res_Type) );
	
	memcpy( restype->id, id, sizeof(id) );
 
	restype->write = writefunc;
	restype->read  = readfunc;

	// insert into list
	restype->next = res_types;
	res_types = restype;

}

static Res_Type *lookup_res_type( const char typeid[4] ) {

	for( struct Res_Type *type = res_types; NULL!=type; type=type->next )
		if( 0 == memcmp( type->id, typeid, sizeof(type->id) ) )
			return type;
	return NULL;

}

// Resource search paths //////////////////////////////////////////////////////

struct Res_Path {

	const char *scheme;
	const char *path;

	struct Res_Path *next;

};

static struct Res_Path  absolute_path = { "file", "", NULL };
static struct Res_Path* resource_paths = &absolute_path;

static char* replace( const char *s, const char *begin, const char *end, const char *subst) {

	//      0         1
	//      01234567890
	//      ^     ^
	// s = "${PWD}/blah";
	const int remove_length = end - begin;
	const int replace_length = strlen(subst);
	const int new_length = strlen(s) - remove_length + replace_length;
	char *new_s = malloc( new_length + 1 );

	// Copy up until the beginning of the region to be replace
	char *new_sp = new_s;
	while( new_sp < begin )
		*new_sp++ = *s++;

	// Copy the substitution 
	while( '\0' != *subst )
		*new_sp++ = *subst++;

	// Resume copying from the source after the replacement region
	s = end;
	while( new_sp < new_s + new_length )
		*new_sp++ = *s++;
	*new_sp = '\0';

	return new_s;
}

static const char* shell_expand( const char* s ) {

	char* expansion = strdup( s );
	const char* dollar = strchr(expansion, '$');
	while( NULL != dollar ) {

		switch( *(dollar + 1) ) {
			
		case '$': {
			// dollar = "$$...."
			char* replaced = replace( expansion, dollar, dollar + 2, "$" );
			free( expansion );
			expansion = replaced;
			break;
		}
		case '{': {
			// Parse the varname and get its value
			const char* var_front = dollar + 2;
			const char* var_end = strchr( var_front, '}' );
			if( var_end ) {
				char var[ var_end - var_front + 1 ];
				strncpy( var, var_front, var_end - var_front );
				var[ var_end-var_front ] = '\0';
			
				const char* value = getenv(var);
				char* replaced = replace( expansion, dollar, var_end + 1, value ? value : "" );
				free(expansion ); 
				expansion = replaced;
			}
			break;
		}

		default: {
			// Just replace the $ with nothing
			replace( expansion, dollar, dollar + 1, "");
			break;
		}
		}

		// Keep going until no more $'s
		dollar = strchr( expansion, '$' );
		
	}

	return expansion;
}

void       add_Res_path( const char* scheme, const char* pathspec ) {

	const char *path = shell_expand( pathspec );
	int trailslash   = ('/' == path[ strlen(path)-1 ]) ?  0 : 1;

	struct Res_Path* respath = malloc( sizeof(struct Res_Path)
	                                   + strlen(scheme)+1
	                                   + strlen(path) + 1 
	                                   + trailslash );
	

	char *schemebuf = (pointer)respath + sizeof(struct Res_Path);
	char *pathbuf   = (pointer)schemebuf + strlen(scheme) + 1;

	strcpy( schemebuf, scheme );
	strcpy( pathbuf, path );
	if( trailslash )
		strcat( pathbuf, "/" );
		
	respath->scheme = schemebuf;
	respath->path = pathbuf;

	// Insert into list
	respath->next = resource_paths;
	resource_paths = respath;

	free( (char*)path );

}

static char *resolve_res( const char* name ) {

	const struct Res_Path* respath = resource_paths;
	
	while( respath ) {

		char path [ strlen(respath->path) + strlen(name) + 1 ];

		strcpy( path, respath->path );
		strcat( path, name );

		if( Fs_exists( path ) ) {
			char *retpath = malloc( sizeof(path) );
			strcpy( retpath, path );
			return retpath;
		}

		respath = respath->next;

	}
	
	return NULL;

}

Resource  *new_Res( Resource *parent,
                    const char *name, 
                    const char typeid[4],
                    pointer data ) {

	pointer resbuf = malloc( sizeof(Resource) + strlen(name) + 1 + sizeof(typeid) + 1);
	Resource *res = resbuf;

	// Store name and typeid.
	res->name = resbuf + sizeof(Resource);
	strcpy( res->name, name );
	strcat( res->name, "." );
	strncat( res->name, typeid, sizeof(typeid) );

	res->data = data;
	res->refc = 0;
	
	res->type = lookup_res_type( typeid );

	if( parent ) {
		res->parent = parent;
		res->next = parent->child;
		parent->child = res;
	} else {
		res->parent = NULL;
		res->child = NULL;
		res->next = NULL;
	}

	return res;

}

size_t     write_Res( Resource *res, const char *outdir ) {

	assert( res );
	assert( res->type );
	assert( 0 == strncmp( res->type->id, strchr( res->name, '.' )+1, sizeof(res->type->id) ) );
	assert( outdir );

	// Construct the directory name: outdir/`dirname res->name`
	char *stem = strrchr(res->name, '/');
	char dirname[ strlen(outdir) + 1 + (stem ? (stem - res->name) : 0) + 1 ];
	strcpy( dirname, outdir );
	strcat( dirname, fileSeparator_string );
	strncat( dirname, res->name, stem ? (stem - res->name) : 0 );
	
	if( Fs_mkdirs(dirname) < 0 )
		return -1;

	char path[ strlen(outdir) + 1          // <outdir>/
	           + strlen(res->name)         // <resname>
	           + 1 ];
	strcpy( path, outdir );
	strcat( path, fileSeparator_string );
	strcat( path, res->name );

	FILE *outp = fopen( path, "wb" );
	if( !outp )
		return -1;

	// Write the typeid, followed by the name, then dispatch to write_Resource_f
	size_t len = strlen(res->name);

	fwrite( res->type->id, sizeof(res->type->id), 1, outp );
	fwrite( &len, sizeof(len), 1, outp );
	fwrite( res->name, sizeof(char), len, outp );

	res->type->write( res->data, outp );
	
	// Measure size
	size_t sz = ftell( outp );
	fclose( outp );

	// Write children
	for( Resource *child = res->child; child; child=child->next )
		sz += write_Res( child, outdir );

	return sz;

}

Resource   *read_Res( const char *name ) {

	assert( name );

	char *path = resolve_res( name );
	if( !path ) {
		fatal( "Could not resolve resource `%s'.", 
		       name );
		return NULL;
	}

	FILE *inp = fopen( path, "rb" );
	if( !inp ) {
		fatal( "Failed to open resource: %s", path );
		free(path);
		return NULL;
	} else
		free( path );

	// Extract type id
	char typeid[4] = { '\0', '\0', '\0', '\0' };
	strncpy( typeid, strrchr( name, '.' )+1, sizeof(typeid) );

	// Read the type id from file
	char restypeid[4]; fread( restypeid, sizeof(restypeid), 1, inp );
	if( 0 != memcmp( restypeid, typeid, sizeof(typeid) ) ) {
		fatal( "Resource type mismatch: resource `%s' declared type `%.4s' but detected `%.4s'",
		       name, typeid, restypeid );
		fclose( inp );
		return NULL;
	}

	// Lookup typeid
	Res_Type *type = lookup_res_type( typeid );
	if( !type ) {
		fatal( "Cannot read `%s', unknown type: `%.4s'", name, typeid );
		return NULL;
	}

	// Read the name
	size_t len;          fread( &len, sizeof(len), 1, inp );
	char resname[ len ]; fread( resname, sizeof(char), len, inp );
	
	if( strncmp( resname, name, len ) ) {
		error( "Resource name mis-match; resource in file `%s' is named `%s', expected `%s'",
		       path, resname, name );
		fclose( inp );
		return NULL;
	}

	pointer data = type->read( inp );
	fclose( inp );

	return new_Res( NULL, name, typeid, data );

}

Resource *import_Res( const char *name, const char* path ) {

	const char* ext = strrchr(path, '.') + 1;
	struct Importer* imptr = importers;

	while( imptr ) {

		if( 0 == strcmp(ext, imptr->ext) )
			break;

		imptr = imptr->next;

	}
	if( !imptr )
		return NULL;

	FILE *fp = fopen( path, "rb" );

	fseek( fp, 0L, SEEK_END );
	long sz = ftell( fp );
	pointer buf = malloc( sz );
	if( !buf ) {
		fclose( fp );

		error0( "Out of memory");
		return NULL;
	}

	rewind( fp );
	fread( buf, 1, sz, fp );
	fclose( fp );

	Resource *res = imptr->import( name, sz, buf );
	
	free( buf );
	return res;

}
/*
resource_p load_resource_TXT( int size, const void* buf ) {

	char* txt = (char*)alloc( NULL, size + 1 );
	strncpy( txt, (char*)buf, size );
	txt[size] = '\0';

	return create_raw_RES( size, txt, -1 );
	
}
*/

#ifdef __res_core_TEST__

#include <stdio.h>

int main( int argc, char* argv[] ) {

	if( argc < 2 ) {
		printf("Usage:\t%s <path to .txt>\n", argv[0]);
		return 1;
	} else {

		add_Res_path( "file", "${PWD}/res" );

		char* ext = strrchr(argv[1], '.');
		if( ext ) 
			register_loader_RES( ext+1, load_resource_TXT );
		else {
			printf("Please specify a text file with an extension\n");
			return 2;
		}
	}

	const char* txt = resource( char, argv[1] );
	printf("Contents:\n%s\n", txt);

	return 0;
}


#endif
