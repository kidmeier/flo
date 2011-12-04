#include <alloca.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <curl/curl.h>

#include "res.core.h"
#include "sync.once.h"
#include "core.alloc.h"

// Buffers ////////////////////////////////////////////////////////////////////
typedef struct {

	size_t   size;
	void*    data;

	off_t    pos;

} buf_t;

#define DEFAULT_BLK_SIZE  1024*1024
static buf_t* alloc_buf(int blk_size) {
	
	buf_t* buf = new(NULL, buf_t);
	buf->size = blk_size <= 0 ? DEFAULT_BLK_SIZE : blk_size;
	buf->data = malloc( buf->size );
	buf->pos = 0;

	return buf;
}

static size_t resize_buf( buf_t* buf, int sz ) {

	// Next highest multiple of buf->size * 2
	int   new_size = 2 * (buf->size + sz - (buf->size % sz));
	void* new_data = realloc( buf->data, new_size );

		if( !new_data )
			return -1;
		
		buf->size = new_size;
		buf->data = new_data;

		return buf->size;
}

static void free_buf( buf_t* buf ) {

	if( !buf ) return;
	if( buf->data ) free(buf->data);
	delete(buf);

}

// libcurl ////////////////////////////////////////////////////////////////////

static once_t init_libcurl_once = init_ONCE;
static void init_libcurl( void ) { curl_global_init( CURL_GLOBAL_ALL ); }
static __thread CURL* curl = NULL;
static __thread char* curl_error_string = NULL;

static size_t write_curldata_func( void* ptr, size_t sz, size_t n, void* arg ) {

	buf_t* buf = (buf_t*)arg;
	
	if( buf->pos + n*sz >= buf->size ) {

		if( resize_buf( buf, buf->pos + n*sz ) < 0 ) {
			memcpy( buf->data + buf->pos, ptr, buf->size - buf->pos );
			return buf->size - buf->pos;
		}

	}

	memcpy( buf->data + buf->pos, ptr, n*sz );
	buf->pos = buf->pos + n*sz;

	return n*sz;
}

static CURLcode read_url( const char* url, buf_t* buf ) {

	once( init_libcurl );

	if( NULL == curl ) {
		curl = curl_easy_init();
		curl_error_string = (char*)malloc( CURL_ERROR_SIZE + 1 );
	}

	curl_easy_setopt( curl, CURLOPT_WRITEDATA, buf );
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_curldata_func );
	curl_easy_setopt( curl, CURLOPT_URL, url );
	curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, curl_error_string);

	CURLcode ret = curl_easy_perform(curl);
	if( 0 != ret ) {
		fprintf(stderr, "libcurl: %s\n", curl_error_string);
	}

	return ret;

}

// Public API /////////////////////////////////////////////////////////////////

struct resource_loader_s {

	char*           ext;
	load_resource_f loadfunc;

	struct resource_loader_s* next;

};

static struct resource_loader_s* loaders = NULL;
static resource_p cache = NULL;

// Caching ////////////////////////////////////////////////////////////////////

static void insert_cache( resource_p res ) {

	res->prev = NULL;
	res->next = cache;
	if( cache )
		cache->prev = res;
	cache = res;
	
}

static void evict_cache( resource_p res ) {

	if( res->prev ) res->prev->next = res->next;
	if( res->next ) res->next->prev = res->prev;

}

static resource_p hit_cache( const char* name ) {

	resource_p node = cache;
	while( node ) {

		if( 0 == strcmp(name, node->name) )
			return node;

		node = node->next;

	}

	return NULL;

}

void register_loader_RES( const char* ext, load_resource_f loadfunc ) {

	struct resource_loader_s* ldr = new( NULL, struct resource_loader_s );

	ldr->ext = clone_string(ldr, ext);
	ldr->loadfunc = loadfunc;

	// insert into list
	ldr->next = loaders;
	loaders = ldr;

}

// Resource search paths //////////////////////////////////////////////////////

struct resource_path_s {

	const char*             url_prefix;
	struct resource_path_s* next;

};

static struct resource_path_s  absolute_path = { "", NULL };
static struct resource_path_s* resource_paths = &absolute_path;

static char* replace( const char* s, const char* begin, const char* end, const char* subst) {

	//      0         1
	//      01234567890
	//      ^     ^
	// s = "${PWD}/blah";
	const int remove_length = end - begin;
	const int replace_length = strlen(subst);
	const int new_length = strlen(s) - remove_length + replace_length;
	char* new_s = alloc(NULL, new_length + 1);

	// Copy up until the beginning of the region to be replace
	char* new_sp = new_s;
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

	const char* expansion = clone_string( NULL, s );

	const char* dollar = strchr(expansion, '$');
	while( NULL != dollar ) {

		switch( *(dollar + 1) ) {
			
		case '$': {
			// dollar = "$$...."
			const char* replaced = replace( expansion, dollar, dollar + 2, "$" );
			delete(expansion); expansion = replaced;
			break;
		}
		case '{': {
			// Parse the varname and get its value
			const char* var_front = dollar + 2;
			const char* var_end = strchr( var_front, '}' );
			if( var_end ) {
				char* var = alloca( var_end - var_front + 1);
				strncpy( var, var_front, var_end - var_front );
				var[ var_end-var_front ] = '\0';
			
				const char* value = getenv(var);
				const char* replaced = replace( expansion, dollar, var_end + 1, value ? value : "" );
				delete(expansion); expansion = replaced;
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

void add_path_RES( const char* scheme, const char* pathspec ) {

	struct resource_path_s* respath = new( NULL, struct resource_path_s );
	pathspec = shell_expand(pathspec);

	int trailslash = '/' == pathspec[ strlen(pathspec)-1 ] ?  0 : 1;

	//                                                   "://"                         <'/'> '\0'
	respath->url_prefix = alloc( respath, strlen(scheme) + 3 + strlen(pathspec) + trailslash + 1 );
	strcpy( (char*)respath->url_prefix, scheme ),
		strcat( (char*)respath->url_prefix, "://"),
		strcat( (char*)respath->url_prefix, pathspec),
		trailslash ? strcat( (char*)respath->url_prefix, "/" ) : (void)0;
		
	// Insert into list
	respath->next = resource_paths;
	resource_paths = respath;

	delete(pathspec);
}

resource_p create_raw_RES( int size, void* data, msec_t expiry ) {

	resource_p res = new( NULL, resource_t );

	res->size = size;
	res->data = data;
	adopt(res, res->data);

	// Timestamp it
	res->timestamp = milliseconds();

	// Compute the expiry if given, else set to 0
	if( expiry > 0 )
		res->expiry = milliseconds() + expiry;
	else
		res->expiry = 0;
	
	res->refcount = 0;
	return res;

}

static resource_p resolve_res( const char* res_name, int size_hint, struct resource_loader_s* ldr ) {
	const struct resource_path_s* path = resource_paths;
	
	// If we have the scheme specifier then we start with absolute_path
	if( strstr(res_name, "://") )
		path = &absolute_path;

	while( path ) {

		char* url = alloca( strlen(path->url_prefix) + strlen(res_name) + 1 );
		strcpy( url, path->url_prefix ),
			strcat( url, res_name );

		buf_t* buf = alloc_buf( size_hint );
		
		if( 0 == read_url( url, buf ) ) {
			resource_p res = ldr->loadfunc( buf->pos, buf->data );
			free_buf(buf);

			return res;
		}

		path = path->next;

	}	
	
	return NULL;

}	


resource_p load_RES( const char* res_name, int size_hint ) {

	const char* ext = strrchr(res_name, '.') + 1;
	struct resource_loader_s* ldr = loaders;

	while( ldr ) {

		if( 0 == strcmp(ext, ldr->ext) )
			break;

		ldr = ldr->next;

	}

	if( ldr ) {

		resource_p res = resolve_res( res_name, size_hint, ldr );
		if( res ) {
			
			res->name = clone_string( res, res_name );
			insert_cache(res);

		}

		return res;
	}

	return NULL;

}

resource_p get_RES( const char* name ) {

	resource_p res = hit_cache(name);

	if( !res )
		res = load_RES(name, -1);

	if( res )
		res->refcount++;
	return res;

}

void       put_RES( resource_p res ) {

	res->refcount--;

	if( 0 == res->refcount ) {

		msec_t t = milliseconds();

		if( res->expiry && t > res->expiry ) {

			evict_cache(res);
			delete(res);

		}
		
	}

}

resource_p load_resource_TXT( int size, const void* buf ) {

	char* txt = (char*)alloc( NULL, size + 1 );
	strncpy( txt, (char*)buf, size );
	txt[size] = '\0';

	return create_raw_RES( size, txt, -1 );
	
}

#ifdef __res_core_TEST__

#include <stdio.h>

int main( int argc, char* argv[] ) {

	if( argc < 2 ) {
		printf("Usage:\t%s <path to .txt>\n", argv[0]);
		return 1;
	} else {

		add_path_RES( "file", "${PWD}/res" );

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
