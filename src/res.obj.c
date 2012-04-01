#include <stdlib.h>

#include "control.maybe.h"
#include "core.log.h"
#include "parse.core.h"
#include "r.mesh.h"
#include "res.core.h"
#include "res.obj.h"

#define INITIAL_VERTS   1024
#define INITIAL_UVS     1024
#define INITIAL_NORMALS 1024
#define INITIAL_TRIS    1024

struct Obj_extents {

	int max_verts;
	int max_uvs;
	int max_normals;
	int max_tris;

};

static parse_p ff( parse_p P ) {

	P = skipws(P);
	if( '#' == lookahead(P,0) )
		return skipws( parsync(P, '\n', NULL) );

	return P;

}

static Mesh *alloc_mesh( struct Obj_extents *extents ) {

	Mesh* mesh = malloc( sizeof(Mesh) );
	if( !mesh )
		return NULL;

	extents->max_verts   = INITIAL_VERTS;
	extents->max_uvs     = INITIAL_UVS;
	extents->max_normals = INITIAL_NORMALS;
	extents->max_tris    = INITIAL_TRIS;

	mesh->n_verts   = 0;
	mesh->n_uvs     = 0;
	mesh->n_normals = 0;
	mesh->n_tris    = 0;

	mesh->verts   = calloc( 3 * extents->max_verts,   sizeof(float) );
	mesh->uvs     = calloc( 2 * extents->max_uvs,     sizeof(float) );
	mesh->normals = calloc( 3 * extents->max_normals, sizeof(float) );
	mesh->tris    = calloc( 3 * extents->max_tris,    sizeof(Mesh_Vertex)   );

	mesh->bounds.mins = (float4){ 0.f, 0.f, 0.f, 1.f };
	mesh->bounds.maxs = (float4){ 0.f, 0.f, 0.f, 1.f };

	return mesh;

}

static int expand_obj( void **ary, int size, int arity, int *extent ) {

	void *expansion = realloc( (*ary), 2 * size * arity * (*extent) );
	if( !expansion )
		return -1;

	(*ary)    = expansion;
	(*extent) = 2 * (*extent);

	return 0;

}

static int write_float2( float **ary, int *n, int *extent,
                         float f0, float f1 ) {

	if( (*n) >= (*extent) ) {

		int rc = expand_obj( (void**)ary, sizeof(float), 2, extent );
		if( rc < 0 )
			return rc;

	}

	(*ary)[ 2*(*n) + 0 ] = f0;
	(*ary)[ 2*(*n) + 1 ] = f1;
	(*n)++;

	return (*n);

}

static int write_float3( float **ary, int *n, int *extent,
                         float f0, float f1, float f2 ) {

	if( (*n) >= (*extent) ) {

		int rc = expand_obj( (void**)ary, sizeof(float), 3, extent );
		if( rc < 0 )
			return rc;

	}

	(*ary)[ 3*(*n) + 0 ] = f0;
	(*ary)[ 3*(*n) + 1 ] = f1;
	(*ary)[ 3*(*n) + 2 ] = f2;
	(*n)++;

	return (*n);

}

static int write_face( Mesh *mesh, struct Obj_extents *extents, 
                       int v0, int uv0, int n0,
                       int v1, int uv1, int n1,
                       int v2, int uv2, int n2 ) {

	if( mesh->n_tris >= extents->max_tris ) {

		int rc = expand_obj( (void**)&mesh->tris, sizeof(Mesh_Vertex), 3, &extents->max_tris );
		if( rc < 0 )
			return rc;

	}

	// Convert reverse into forward references
	v0 = (v0 < 0) ? mesh->n_verts + v0 + 1 : v0;
	v1 = (v1 < 0) ? mesh->n_verts + v1 + 1 : v1;
	v2 = (v2 < 0) ? mesh->n_verts + v2 + 1 : v2;

	assert( v0 > 0 && v0 <= mesh->n_verts );
	assert( v1 > 0 && v1 <= mesh->n_verts );
	assert( v2 > 0 && v2 <= mesh->n_verts );

	uv0 = (uv0 < 0) ? mesh->n_uvs + uv0 + 1 : uv0;
	uv1 = (uv1 < 0) ? mesh->n_uvs + uv1 + 1 : uv1;
	uv2 = (uv2 < 0) ? mesh->n_uvs + uv2 + 1 : uv2;

	assert( uv0 >= 0 && uv0 <= mesh->n_uvs );
	assert( uv1 >= 0 && uv1 <= mesh->n_uvs );
	assert( uv2 >= 0 && uv2 <= mesh->n_uvs );

	n0 = (n0 < 0) ? mesh->n_normals + n0 + 1 : n0;
	n1 = (n1 < 0) ? mesh->n_normals + n1 + 1 : n1;
	n2 = (n2 < 0) ? mesh->n_normals + n2 + 1 : n2;

	assert( n0 >= 0 && n0 <= mesh->n_normals );
	assert( n1 >= 0 && n1 <= mesh->n_normals );
	assert( n2 >= 0 && n2 <= mesh->n_normals );

	mesh->tris[ 3*mesh->n_tris + 0 ].v = v0-1;
	mesh->tris[ 3*mesh->n_tris + 0 ].uv = uv0-1;
	mesh->tris[ 3*mesh->n_tris + 0 ].n = n0-1;

	mesh->tris[ 3*mesh->n_tris + 1 ].v = v1-1;
	mesh->tris[ 3*mesh->n_tris + 1 ].uv = uv1-1;
	mesh->tris[ 3*mesh->n_tris + 1 ].n = n1-1;

	mesh->tris[ 3*mesh->n_tris + 2 ].v = v2-1;
	mesh->tris[ 3*mesh->n_tris + 2 ].uv = uv2-1;
	mesh->tris[ 3*mesh->n_tris + 2 ].n = n2-1;

	mesh->n_tris++;
	return mesh->n_tris;

}

static parse_error_p parse_face_vertex( parse_p P, int *v, int *uv, int *n ) {

	if( !parsok( integer(ff(P), v) ) )
		return parserr( P, "expected [int]" );

	if( trymatchc(P, '/') ) {

		// int//int ?
		if( trymatchc(P, '/') ) {

			if( !parsok( integer(P, n) ) )
				return parserr( P, "expected int//[int]" );

			// All good
			return NULL;

		} else {

			// int/int...
			if( !parsok( integer(P, uv) ) )
				return parserr( P, "expected int/[int]" );

			// int/int/int ?
			if( trymatchc(P, '/') ) {
				
				if( !parsok( integer(P, n) ) )
					return parserr( P, "expected int/int/[int]" );
				
			}
		}
	} 

	return NULL;

}

static parse_error_p parse_face( parse_p P, Mesh *mesh, 
                                 struct Obj_extents *extents ) {

	int v0, v1, v2, v3;
	int uv0 = 0, uv1 = 0, uv2 = 0, uv3 = 0;
	int n0 = 0, n1 = 0, n2 = 0, n3 = 0;
	
	// Parse first triangle
	parse_error_p err = parse_face_vertex(P, &v0, &uv0, &n0);
	err = maybe(err, != NULL, parse_face_vertex(P, &v1, &uv1, &n1) );
	err = maybe(err, != NULL, parse_face_vertex(P, &v2, &uv2, &n2) );
	
	if( err ) 
		return err;

	if( write_face(mesh, extents, v0, uv0, n0, v1, uv1, n1, v2, uv2, n2) < 0 )
		return parserr(P, "out of memory");

	// Continue until we have no more triangles
	while( 1 ) {

		while( trymatchc( P, ' ' ) || trymatchc( P, '\t' ) || trymatchc( P, '\r' ) )
			;

		if( trymatchc( P, '\n' ) )
			break;

		err = parse_face_vertex( P, &v3, &uv3, &n3 );
		if( err )
			return err;

		if( write_face(mesh, extents, v0, uv0, n0, v2, uv2, n2, v3, uv3, n3) < 0 )
			return parserr(P, "out of memory");

		v2 = v3;
		uv2 = uv3;
		n2 = n3;

	}

	ff(P);
	return err;

}

static parse_error_p parse_vertex( parse_p P, Mesh *mesh, 
                                   struct Obj_extents *extents ) {

	float v[3];

	for( int i=0; i<3; i++ ) {

		if( !parsok( decimalf(ff(P), &v[i]) ) )
			return parserr( P, "expected [float]" );

	}

	if( write_float3( &mesh->verts, &mesh->n_verts, &extents->max_verts,
	                  v[0], v[1], v[2] ) < 0 )
		return parserr( P, "out of memory" );

	expand_AABB( &mesh->bounds, (float4){ v[0], v[1], v[2], 1.f } );
	return NULL;
}

static parse_error_p parse_uv( parse_p P, Mesh *mesh, 
                               struct Obj_extents *extents ) {

	float uv[2];

	for( int i=0; i<2; i++ ) {

		if( !parsok( decimalf(ff(P), &uv[i]) ) )
			return parserr( P, "expected [float]" );

	}

	if( write_float2( &mesh->uvs, &mesh->n_uvs, &extents->max_uvs,
	                  uv[0], uv[1] ) < 0 )
		return parserr( P, "out of memory" );

	
	// Some OBJ files contain 3d tex coords; our rendering model only uses
	// 2d coords, so we'll just ignore the 3rd component if it exists.
	float dummy;

	decimalf( ff( P ), &dummy );
	return NULL;
}

static parse_error_p parse_normal( parse_p P, Mesh *mesh, 
                                   struct Obj_extents *extents ) {

	float n[3];

	for( int i=0; i<3; i++ ) {

		if( !parsok( decimalf(ff(P), &n[i]) ) )
			return parserr( P, "expected [float]" );

	}

	if( write_float3( &mesh->normals, &mesh->n_normals, &extents->max_normals,
	                  n[0], n[1], n[2]) < 0 )
		return parserr( P, "out of memory" );
	
	return NULL;
}

// ////////////////////////////////////////////////////////////////////////////

Resource *import_Obj( const char *name, size_t sz, const pointer data ) {

	parse_p P = new_buf_PARSE( sz, (const char*)data );
	
	const char* statements[] = {
		"f",
		"g",
		"mtllib",
		"o",
		"s",
		"usemtl",
		"vn",
		"vt",
		"v",
		"#", // comment
	};
	int optc = sizeof(statements) / sizeof(statements[0]);

	struct Obj_extents extents;
	Mesh *mesh = alloc_mesh( &extents );

	while( parsok(P) && !parseof(P) ) {

		int choice;
		
		parselect( ff(P), optc, statements, &choice );
		if( !parsok(P) ) {
			parserr(P, "unsupported .obj statement");
			parsync(P, '\n', NULL);

			continue;
		}

		switch( choice ) {

		case 0: // f
			parse_face( P, mesh, &extents );
			break;

		case 1: // g
			// no-op for now
			parsync( P, '\n', NULL );
			break;

		case 2: // mtllib
			// no-op for now
			parsync( P, '\n', NULL );
			break;

		case 3: // o
			// no-op for now
			parsync( P, '\n', NULL );
			break;

		case 4: // s
			// no-op for now
			parsync( P, '\n', NULL );
			break;

		case 5: // usemtl
			// no-op for now
			parsync( P, '\n', NULL );
			break;

		case 6: // vn
			parse_normal( P, mesh, &extents );
			break;

		case 7: // vt
			parse_uv( P, mesh, &extents );
			break;
			
		case 8: // v
			parse_vertex( P, mesh, &extents );
			break;

		case 9: // #
			parsync( P, '\n', NULL );
			break;

		default:
			fatal( "Unhandled .obj statement type: %d", choice );
			break;
		}

	}

	if( parserrc(P) ) {

		error0("Error(s) loading .obj:");
		for( int i=0; i<parserrc(P); i++ ) {

			parse_error_p err = parserri(P,i);
			error( "\t%s", err->line );
			error( "\tline %d.%d: %s", err->lineno, err->col, err->msg );

		}

	}

	if( !parsok(P) ) {

		destroy_PARSE( P );
		return NULL;

	} else {

		info( "import_Obj: %s", name );
		dump_Mesh_info( mesh );

	}

	destroy_PARSE( P );
	return new_Res( NULL,
	                name,
	                "mesh",
	                mesh );
	
}

#ifdef __res_obj_TEST__

#include <string.h>

int main( int argc, char* argv[] ) {

	register_loader_RES( "obj", load_resource_Obj );
	add_path_RES( "file", "${PWD}" );

	if( argc < 2 ) {
		printf("Usage:\t%s <path to .obj>\n", argv[0]);
		return 1;
	}

	Mesh *mesh = resource(Mesh,argv[1]);
	if( !mesh ) {
		printf("Failed to load: %s\n", argv[1]);
		return 255;
	}

	printf(".objstat %s:\n\n", argv[1]);
	printf("n_verts:\t%d\n", mesh->n_verts);
	printf("n_uvs:\t%d\n", mesh->n_uvs);
	printf("n_normals:\t%d\n", mesh->n_normals);
	printf("n_tris:\t%d\n", mesh->n_tris);

	return 0;
}

#endif
