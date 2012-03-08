#include <stdlib.h>
#include <string.h>

#include "parse.core.h"
#include "r.skel.h"
#include "res.core.h"
#include "res.md5.h"
#include "math.vec.h"

static parse_p ff( parse_p P ) {

	P = skipws(P);
	if( '/' == lookahead(P, 0)
	    && '/' == lookahead(P, 1) )
		return skipws(parsync( P, '\n', NULL));

	return P;

}

static Skeleton *parse_header( parse_p P ) {

	if( !parsok( match( ff(P), "MD5Version") ) )
		return NULL;

	int version; integer( P, &version );
	if( !parsok(P) || 10 != version ) 
		return NULL;

	if( !parsok( match( ff(P), "commandline" ) ) )
		return NULL;

	if( !parsok( qstring( ff(P), NULL ) ) )
		return NULL;

	if( !parsok( match( ff(P), "numJoints" ) ) )
		return NULL;

	int n_joints; integer( ff(P), &n_joints );
	if( !parsok(P) ) 
		return NULL;

	if( !parsok( match( ff(P), "numMeshes" ) ) ) 
		return NULL;

	int n_meshes; integer( ff(P), &n_meshes );
	if( !parsok(P) ) 
		return NULL;

	Skeleton * skel = malloc( sizeof(Skeleton) );

//	skel->version = version;
	skel->n_joints = n_joints;
	skel->n_meshes = n_meshes;

	return skel;

}

static parse_p parse_float3( parse_p P, float4 *pos) {
	
	matchc( ff(P), '(' );
	decimalf( ff(P), &pos->x );
	decimalf( ff(P), &pos->y );
	decimalf( ff(P), &pos->z ); 
	matchc( ff(P), ')' ); 

	return P;

}

static parse_p parse_quat( parse_p P, float4 *q ) {

	parse_float3( P, q );

  float w = 1.0 - (q->x*q->x) - (q->y*q->y) - (q->z*q->z);
  if( w < 0.0f )
    q->w = 0.0f;
  else
    q->w = -sqrt(w);

  return P;

}

static Skel_Joint *parse_joints( parse_p P, Skeleton *skel ) {

	if( !parsok( match( ff(P), "joints" ) ) ) return NULL;
	if( !parsok( matchc( ff(P), '{' ) ) ) return NULL;

	Skel_Joint * joints = calloc( skel->n_joints, sizeof(Skel_Joint) );
	for( int i=0; i<skel->n_joints; i++ ) {
		
		Skel_Joint *joint = &joints[i];
		int parent_idx;

		qstring( ff(P), (char**)&joint->name );
		integer( ff(P), &parent_idx );
		if( parent_idx >= 0 )
			joint->parent = &joints[parent_idx];

		parse_float3( ff(P), &joint->p ); joint->p.w = 1.0f;
		parse_quat( ff(P), &joint->qr );

		if( !parsok(P) ) {
			free( joints );
			return NULL;
		}

	}

	if( !parsok( matchc( ff(P), '}' ) ) ) {
		free( joints );
		return NULL;
	}

	return joints;

}

static
parse_p parse_texcoord( parse_p P, float *s, float *t ) {
	
	matchc  ( ff(P), '(' );
	decimalf( ff(P), s );
	decimalf( ff(P), t );
	matchc  ( ff(P), ')' );
	
	return P;
	
}

static parse_error_p parse_mesh( parse_p P, Skel_Joint *joints, Skel_Mesh *mesh ) {

	qstring( ff( match( ff(P), "shader"   ) ), (char**)&mesh->shader );
	if( !parsok(P) )
		return parserr( P, "Expected: `shader <string>'" );
	
	integer( ff( match( ff(P), "numverts" ) ), &mesh->n_verts );
	if( !parsok(P) )
		return parserr( P, "Expected: `numverts integer'");
	
	mesh->verts = calloc( mesh->n_verts, sizeof(Skel_Vertex) );
	for( int i=0; i<mesh->n_verts; i++ ) {
		
		int idx;
		
		match( ff(P), "vert" );
		integer( ff(P), &idx );
		parse_texcoord( ff(P), &mesh->verts[idx].s, &mesh->verts[idx].t );
		integer( ff(P), &mesh->verts[idx].start_weight );
		integer( ff(P), &mesh->verts[idx].n_weights );
		
		if( !parsok(P) )
			return parserr( P, "Expected: `vert <int> (<float> <float>) <int> <int>'" );
		
	}
	
	integer( ff( match( ff(P), "numtris" ) ), &mesh->n_tris );
	if( !parsok(P) )
		return parserr( P, "Expected: `numtris <int>'" );
	
	mesh->tris = calloc( 3 * mesh->n_tris, sizeof(int) );
	for( int i=0; i<mesh->n_tris; i++ ) {
		
		int idx; 
		
		match( ff(P), "tri" );
		integer( ff(P), &idx );
		integer( ff(P), &mesh->tris[3*idx + 0] );
		integer( ff(P), &mesh->tris[3*idx + 1] );
		integer( ff(P), &mesh->tris[3*idx + 2] );
		
		if( !parsok(P) ) 
			return parserr( P, "Expected: `tri <int> <int> <int> <int>'" );
		
	}
	
	integer( ff( match( ff(P), "numweights" ) ), &mesh->n_weights );
	if( !parsok(P) )
		return parserr( P, "Expected: `numweights <int>'" );
	
	mesh->weights = calloc( mesh->n_weights, sizeof(Skel_Weight) );
	for( int i=0; i<mesh->n_weights; i++ ) {
		
		int idx, joint;
		
		match( ff(P), "weight" );
		integer( ff(P), &idx );
		integer( ff(P), &joint ); mesh->weights[i].joint = &joints[joint];
		decimalf( ff(P), &mesh->weights[i].bias );
		parse_float3( ff(P), &mesh->weights[i].pos ); mesh->weights[i].pos.w = 1.0f;
		
		if( !parsok(P) )
			return parserr( P, "Expected: `weight <int> <int> <float> (<float> <float> <float>)'" );
		
	}
	
	// No errors
	return NULL;
}

static Skel_Mesh *parse_meshes( parse_p P, Skeleton *skel ) {

	Skel_Mesh *meshes = calloc( skel->n_meshes, sizeof(Skel_Mesh) );
	for( int i=0; i<skel->n_meshes; i++ ) {

		match ( ff(P), "mesh" );
		matchc( ff(P), '{' );

		if( !parsok(P) ) {

			parserr( P, "Expected: `mesh {'" );
			goto error;

		}

		Skel_Mesh *mesh = &meshes[i];

		parse_error_p err = parse_mesh( P, skel->joints, mesh );
		if( err )
			goto error;

		if( !parsok( matchc( ff(P), '}' ) ) ) {
			parserr( P, "Expected: `}'" );
			goto error;
		}

	}

	return meshes;

error:
	free( meshes );
	return NULL;

}

static
Skeleton * parse_md5_model( parse_p P ) {

	Skeleton * skel = parse_header(P);

	if( skel ) {
		skel->joints = parse_joints(P, skel);
		skel->meshes = parse_meshes(P, skel);
	}

	return skel;

}

// Public API /////////////////////////////////////////////////////////////////

Resource *import_MD5( const char *name, size_t sz, const pointer data ) {

	parse_p P = new_buf_PARSE( sz, (const char*)data );

	// Parse the structure
	Skeleton * skel = parse_md5_model( P );

	if( !parsok(P) ) {

		free( skel );
		destroy_PARSE(P);

		return NULL;

	}

	// Compute its size
/*
	size_t size = sizeof(Skeleton);
	
	for( int i=0; i<skel->n_joints; i++ ) {

		Skel_Joint * joint = &skel->joints[i];
		size = size + sizeof(Skel_Joint)
			+ strlen(joint->name) + 1;

	}

	for( int i=0; i<skel->n_meshes; i++ ) {

		Skel_Mesh * mesh = &skel->meshes[i];
		size = size + sizeof(Skel_Mesh)
			+ strlen(mesh->shader) + 1
			+ mesh->n_verts * sizeof(Skel_Vertex)
			+ 3 * mesh->n_tris * sizeof(mesh->tris[0])
			+ mesh->n_weights * sizeof(Skel_Weight);

	}
*/
	
	// Create the resource
	return new_Res( NULL, "skel", name, skel );

}

#ifdef __res_md5_TEST__

#include <string.h>

int main( int argc, char* argv[] ) {

	register_loader_RES( "md5mesh", load_resource_MD5 );
	add_path_RES( "file", "${PWD}" );

	if( argc < 2 ) {
		printf("Usage:\t%s <path to .md5mesh>\n", argv[0]);
		return 1;
	}

	Skeleton * skel = resource(Skeleton,argv[1]);
	if( !skel ) {
		printf("Failed to load: %s\n", argv[1]);
		return 255;
	}

	printf("md5stat %s:\n\n", argv[1]);
	printf("version:\t%d\n", skel->version);
	printf("n_joints:\t%d\n", skel->n_joints);
	printf("n_meshes:\t%d\n", skel->n_meshes);

	printf("joints:\n");
	for( int i=0; i<skel->n_joints; i++ ) {

		Skel_Joint * joint = &skel->joints[i];	
		printf("\t% 3d: %s (%s)\n", i, joint->name, joint->parent ? joint->parent->name : "none");

	}

	printf("meshes:\n");
	for( int i=0; i<skel->n_meshes; i++ ) {

		Skel_Mesh * mesh = &skel->meshes[i];
		printf("\t% 3d: verts: % 4d\ttris: % 4d\tweights: % 4d\n", i, mesh->n_verts, mesh->n_tris, mesh->n_weights);

	}

	return 0;
}

#endif
