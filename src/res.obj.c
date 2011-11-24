#include "control.maybe.h"
#include "core.alloc.h"
#include "core.log.h"
#include "parse.core.h"
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

static Obj_res *alloc_obj( struct Obj_extents *extents ) {

	Obj_res* obj = new( NULL, Obj_res );
	if( !obj )
		return NULL;

	extents->max_verts   = INITIAL_VERTS;
	extents->max_uvs     = INITIAL_UVS;
	extents->max_normals = INITIAL_NORMALS;
	extents->max_tris    = INITIAL_TRIS;

	obj->n_verts   = 0;
	obj->n_uvs     = 0;
	obj->n_normals = 0;
	obj->n_tris    = 0;

	obj->verts   = new_array( obj, float, extents->max_verts   );
	obj->uvs     = new_array( obj, float, extents->max_uvs     );
	obj->normals = new_array( obj, float, extents->max_normals );
	obj->tris    = new_array( obj, int  , extents->max_tris    );

	return obj;

}

static int expand_obj( Obj_res *obj, void **ary, int size, int arity, int *extent ) {

	abandon( obj, (*ary) );
	
	void *expansion = alloc( obj, 2 * size * arity * (*extent) );
	if( !expansion )
		return -1;

	memcpy( expansion, (*ary), size * arity * (*extent) );
	delete( (*ary) );

	(*ary)    = expansion;
	(*extent) = 2 * (*extent);

	return 0;

}

static int write_float2( Obj_res* obj, float **ary, int *n, int *extent,
                         float f0, float f1 ) {

	if( (*n) >= (*extent) ) {

		int rc = expand_obj( obj, (void**)ary, sizeof(float), 2, extent );
		if( rc < 0 )
			return rc;

	}

	(*ary)[ 2*(*n) + 0 ] = f0;
	(*ary)[ 2*(*n) + 1 ] = f1;
	(*n)++;

	return (*n);

}

static int write_float3( Obj_res* obj, float **ary, int *n, int *extent,
                         float f0, float f1, float f2 ) {

	if( (*n) >= (*extent) ) {

		int rc = expand_obj( obj, (void**)ary, sizeof(float), 3, extent );
		if( rc < 0 )
			return rc;

	}

	(*ary)[ 3*(*n) + 0 ] = f0;
	(*ary)[ 3*(*n) + 1 ] = f1;
	(*ary)[ 3*(*n) + 2 ] = f2;
	(*n)++;

	return (*n);

}

static int write_face( Obj_res *obj, struct Obj_extents *extents, 
                      int v0, int v1, int v2 ) {

	if( obj->n_tris >= extents->max_tris ) {

		int rc = expand_obj( obj, (void**)&obj->tris, 
		                     sizeof(int), 3, &extents->max_tris );
		if( rc < 0 )
			return rc;

	}

	obj->tris[ 3*obj->n_tris + 0 ] = v0;
	obj->tris[ 3*obj->n_tris + 1 ] = v1;
	obj->tris[ 3*obj->n_tris + 2 ] = v2;

	obj->n_tris++;
	return obj->n_tris;

}

static parse_error_p parse_face_vertex( parse_p P, int *v, int *uv, int *n ) {

	if( !parsok( integer(ff(P), v) ) )
		return parserr( P, "expected [integer]" );

	if( trymatchc(P, '/') ) {

		// int//int ?
		if( trymatchc(P, '/') ) {

			if( !parsok( integer(P, n) ) )
				return parserr( P, "expected [integer]" );

			// All good
			return NULL;

		} else {

			// int/int...
			if( !parsok( integer(P, uv) ) )
				return parserr( P, "expected [integer]" );

			// int/int/int ?
			if( trymatchc(P, '/') ) {
				
				if( !parsok( integer(P, n) ) )
					return parserr( P, "expected [integer]" );
				
			}
		}
	}

	return NULL;

}

static parse_error_p parse_face( parse_p P, Obj_res *obj, 
                                 struct Obj_extents *extents ) {

	int v0, v1, v2;
	int uv0, uv1, uv2; // These are ignored, for now; possibly 
	int n0, n1, n2;    // implemented in future.

	parse_error_p err = parse_face_vertex(P, &v0, &uv0, &n0);
	err = maybe(err, != NULL, parse_face_vertex(P, &v1, &uv1, &n1) );
	err = maybe(err, != NULL, parse_face_vertex(P, &v2, &uv2, &n2) );
	
	if( err ) 
		return err;

	if( write_face(obj, extents, v0, v1, v2) < 0 )
		return parserr(P, "out of memory");

	ff(P);
	return err;

}

static parse_error_p parse_vertex( parse_p P, Obj_res *obj, 
                                   struct Obj_extents *extents ) {

	float v[3];

	for( int i=0; i<3; i++ ) {

		if( !parsok( decimalf(ff(P), &v[i]) ) )
			return parserr( P, "expected [float]" );

	}

	if( write_float3( obj, &obj->verts, &obj->n_verts, &extents->max_verts,
	                  v[0], v[1], v[2] ) < 0 )
		return parserr( P, "out of memory" );

	return NULL;
}

static parse_error_p parse_uv( parse_p P, Obj_res *obj, 
                               struct Obj_extents *extents ) {

	float uv[2];

	for( int i=0; i<2; i++ ) {

		if( !parsok( decimalf(ff(P), &uv[i]) ) )
			return parserr( P, "expected [float]" );

	}

	if( write_float2( obj, &obj->uvs, &obj->n_uvs, &extents->max_uvs,
	                  uv[0], uv[1] ) < 0 )
		return parserr( P, "out of memory" );
	
	return NULL;
}

static parse_error_p parse_normal( parse_p P, Obj_res *obj, 
                                   struct Obj_extents *extents ) {

	float n[3];

	for( int i=0; i<3; i++ ) {

		if( !parsok( decimalf(ff(P), &n[i]) ) )
			return parserr( P, "expected [float]" );

	}

	if( write_float3( obj, &obj->normals, &obj->n_normals, &extents->max_normals,
	                  n[0], n[1], n[2]) < 0 )
		return parserr( P, "out of memory" );
	
	return NULL;
}

// ////////////////////////////////////////////////////////////////////////////

resource_p load_resource_Obj( int sz, const void* data ) {

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
		"v"
	};
	int optc = sizeof(statements) / sizeof(statements[0]);

	struct Obj_extents extents;
	Obj_res *obj = alloc_obj( &extents );

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
			parse_face( P, obj, &extents );
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
			parse_normal( P, obj, &extents );
			break;

		case 7: // vt
			parse_uv( P, obj, &extents );
			break;
			
		case 8: // v
			parse_vertex( P, obj, &extents );
			break;

		default:
			fatal( "Unhandled .obj statement type: %d", choice );
			break;
		}

	}

	if( !parsok(P) ) {

		error0("Error loading .obj:");
		for( int i=0; i<parserrc(P); i++ ) {

			parse_error_p err = parserri(P,i);
			error( "\t%s", err->line );
			error( "\tline %d.%d: %s", err->lineno, err->col, err->msg );

		}
		delete(P);
		return NULL;

	}

	delete(P);
	return create_raw_RES( sizeof(Obj_res) 
	                       + 3 * sizeof(float) * extents.max_verts
	                       + 3 * sizeof(float) * extents.max_normals
	                       + 2 * sizeof(float) * extents.max_uvs
	                       + 3 * sizeof(int)   * extents.max_tris,
	                       obj, 
	                       resNeverExpire );

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

	Obj_res *obj = resource(Obj_res,argv[1]);
	if( !obj ) {
		printf("Failed to load: %s\n", argv[1]);
		return 255;
	}

	printf(".objstat %s:\n\n", argv[1]);
	printf("n_verts:\t%d\n", obj->n_verts);
	printf("n_uvs:\t%d\n", obj->n_uvs);
	printf("n_normals:\t%d\n", obj->n_normals);
	printf("n_tris:\t%d\n", obj->n_tris);

	return 0;
}

#endif
