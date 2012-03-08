#include <stdio.h>
#include <string.h>

#include "res.core.h"
#include "res.spec.h"

static int usage( const char* arg0, FILE *fp ) {

	fprintf( fp, "Usage: %s <base-dir> <output-dir> [resource1 [resource2 [...]]]\n", arg0);
	return -1;

}

static int dumpInfo( FILE *fp, Resource *res ) {

	char typeid[4]; memcpy( typeid, res->type->id, sizeof(typeid) );

	fprintf( fp, "\tresource: %40s\n", res->name );
	int count = 1;
	for( Resource *child=res->child; child; child=child->next )
		count += dumpInfo( fp, child );

	return count;

}

#define RES_SPEC "etc/res.import.spec"
int main( int argc, char *argv[] ) {

	load_Res_spec( RES_SPEC );

	if( argc < 3 )
		return usage( argv[0], stderr );

	FILE *loginfo = stdout;

	int count = 0;
	size_t sz = 0;

	const char *basedir = argv[1];
	const char *outdir  = argv[2];
	

	for( int i=3; i<argc; i++ ) {

		fprintf( loginfo, "importing: %s\n", argv[i] );

		char *ext = strrchr( argv[i], '.' );
		if( !ext )
			ext = argv[i] + strlen(argv[i]);

		char name[ ext - argv[i] + 1 ];
		char path[ strlen(basedir) + 1 + strlen(argv[i]) + 1 ];

		strncpy( name, argv[i], ext - argv[i] );
		name[ ext - argv[i] ] = '\0';

		strcpy( path, basedir );
		strcat( path, "/" );
		strcat( path, argv[i] );

		Resource *res = import_Res( name, path );
		if( !res )
			fprintf( stderr, "Failed to import resource: %s\n", path );
		else
			count += dumpInfo( loginfo, res );

		if( res )
			sz += write_Res( res, outdir );

	}

	fprintf( loginfo, "imported %d resources (%zd bytes)\n", count, sz );
	return 0;

}
