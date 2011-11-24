#include <alloca.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "control.maybe.h"
#include "core.alloc.h"
#include "core.log.h"
#include "parse.core.h"
#include "res.core.h"
#include "res.spec.h"

#include "sys.dll.h"

static void addspec( const char* ext, const char* module, const char* entry ) {

	void* dll = open_DLL( module );
	void* func = maybe( dll, == NULL, lookup_DLL( dll, entry ) );

	if( func ) {
		debug("addspec: %s %s %s", ext, module, entry);
		register_loader_RES( ext+1, (load_resource_f)func );
	} else {
		debug("addspec: failed to resolve %s(%s): %s", module, entry, dlerror());
	}

}

int load_RES_spec( const char* spec ) {

	FILE* fp = fopen( spec, "r" );
	if( !fp ) 
		return -1;

	void*   pool = new_pool(NULL);

	// Determine file length
	fseek( fp, 0, SEEK_END ); 
	long length = ftell( fp ); 
	fseek( fp, 0, SEEK_SET );

	// Read in the file
	char* buf = alloc( pool, length );
	size_t bytes = fread( buf, sizeof(char), length, fp );
	fclose(fp);

	if( bytes != length ) {
		delete(pool);
		return -1;
	}

	// Begin parsing
	parse_p P = new_buf_PARSE( length, buf );
	while( !parseof(P) && parsok(P) ) {

		char* ext;
		char* module;
		char* entry;

		P = string( skipws(P), pool, isspace, &ext );
		P = string( skipws(P), pool, isspace, &module );
		P = matchc( string( skipws(P), pool, isspace, &entry ), '\n' );
		
		if( parsok(P) )
		    addspec( ext, module, entry );
		else {
			P = parsync( P, '\n', NULL );
		}
		
	}

	delete(pool);
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
