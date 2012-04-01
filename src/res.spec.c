#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "control.maybe.h"
#include "core.log.h"
#include "parse.core.h"
#include "res.core.h"
#include "res.spec.h"

#include "sys.dll.h"

static void addimport( const char type[4], const char* ext, const char* module, const char* entry ) {
	dll_t dll = open_DLL( module );
	void* func = maybe( dll, == NULL, lookup_DLL( dll, entry ) );

	if( func ) {
		debug("addimport: %.4s %s %s %s", type, ext, module, entry);
		register_Res_importer( type, ext+1, (import_Resource_f)func );
	} else {
#if defined( feature_POSIX )
		warning("addimport: failed to resolve %s(%s): %s", 
		        module, entry, dlerror());
#else
		warning("addimport: failed to resolve %s(%s)", 
		        module, entry);
#endif
	}

}

static void addtype( const char type[4], const char *module, const char *write, const char *read ) {

	dll_t dll = open_DLL( module );
	void* writefunc = maybe( dll, == NULL, lookup_DLL( dll, write ) );
	void* readfunc = maybe( dll, == NULL, lookup_DLL( dll, read ) );
	
	if( readfunc && writefunc ) {
		debug("addtype: %.4s %s %s %s", type, module, write, read);
		register_Res_type( type, writefunc, readfunc );
	} else {
#if defined( feature_POSIX )
		warning("addtype: failed to resolve %s(%s,%s): %s", 
		        module, write, read, dlerror());
#else
		warning("addtype: failed to resolve %s(%s,%s)", 
		        module, write, read);
#endif
	}

}

int load_Res_spec( const char* spec ) {

	FILE* fp = fopen( spec, "r" );
	if( !fp ) 
		return -1;

	// Determine file length
	fseek( fp, 0, SEEK_END ); 
	long length = ftell( fp ); 
	fseek( fp, 0, SEEK_SET );

	// Read in the file
	char* buf = malloc( length );
	size_t bytes = fread( buf, sizeof(char), length, fp );
	fclose(fp);

	if( bytes != length ) {
		free( buf );
		return -1;
	}

	// Begin parsing
	parse_p P = new_buf_PARSE( length, buf );
	while( !parseof(P) && parsok(P) ) {

		const char *optv[] = { "import", "type" };
		int optc = sizeof(optv) / sizeof(optv[0]);
		int recordtype;

		P = parselect( P, optc, optv, &recordtype );
		if( !parsok(P) ) {
			P = parsync( P, '\n', NULL );
			continue;
		}

		P = matchc( P, ':');
		if( !parsok(P) ) {
			P = parsync( P, '\n', NULL );
			continue;
		}

		switch( recordtype ) {

		case 0: {

			char *type;
			char *ext;
			char *module;
			char *entry;
			
			P = string( skipws(P), isspace, &type );
			P = string( skipws(P), isspace, &ext );
			P = string( skipws(P), isspace, &module );
			P = matchc( string( skipws(P), isspace, &entry ), '\n' );
		
			if( parsok(P) )
				addimport( type, ext, module, entry );
			else
				P = parsync( P, '\n', NULL );

			if( type )   free( type );
			if( ext )    free( ext );
			if( module ) free( module );
			if( entry )  free( entry );

			break;
		}
		case 1: {

			char *type;
			char *module;
			char *freeze;
			char *thaw;
			
			P = string( skipws(P), isspace, &type );
			P = string( skipws(P), isspace, &module );
			P = string( skipws(P), isspace, &freeze );
			P = matchc( string( skipws(P), isspace, &thaw ), '\n' );
		
			if( parsok(P) )
				addtype( type, module, freeze, thaw );
			else
				P = parsync( P, '\n', NULL );

			if( type )   free( type );
			if( module ) free( module );
			if( freeze ) free( freeze );
			if( thaw )   free( thaw );

			break;
		}
			
		default:
			fatal("unhandled record type: %d", recordtype);
			break;
		}
		
	}

	return 0;

}

#ifdef __res_spec_TEST__

resource_p RES_txt_loader( int size, const void* buf ) {
	return NULL;
}

int main( int argc, char* argv[] ) {

	if( argc < 2 ) {
		printf("Usage:\t%s <path to .spec\n", argv[0]);
		return 1;
	}
	load_RES_spec( argv[1] );

	return 0;

}

#endif
