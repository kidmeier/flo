#include "core.alloc.h"
#include "parse.core.h"
#include "res.core.h"
#include "res.md5.h"
#include "math.vec.h"

static
parse_p ff( parse_p P ) {

	P = skipws(P);
	if( '/' == lookahead(P, 0)
	    && '/' == lookahead(P, 1) )
		return skipws(parsync( P, '\n', NULL));

	return P;

}

static 
md5model_p parse_header( parse_p P ) {

	if( !parsok( match( ff(P), "MD5Version") ) )
		return NULL;

	int version; integer( P, &version );
	if( !parsok(P) || 10 != version ) return NULL;
	if( !parsok( match( ff(P), "commandline" ) ) ) return NULL;
	if( !parsok( qstring( ff(P), NULL, NULL ) ) ) return NULL;
	if( !parsok( match( ff(P), "numJoints" ) ) ) return NULL;
	int n_joints; integer( ff(P), &n_joints );
	if( !parsok(P) ) return NULL;
	if( !parsok( match( ff(P), "numMeshes" ) ) ) return NULL;
	int n_meshes; integer( ff(P), &n_meshes );
	if( !parsok(P) ) return NULL;

	md5model_p mdl = new( NULL, md5model_t );

	mdl->version = version;
	mdl->n_joints = n_joints;
	mdl->n_meshes = n_meshes;

	return mdl;

}

static 
parse_p parse_float3( parse_p P, float4* pos) {
	
	matchc( ff(P), '(' );
	decimalf( ff(P), &pos->x );
	decimalf( ff(P), &pos->y );
	decimalf( ff(P), &pos->z ); 
	matchc( ff(P), ')' ); 

	return P;

}

static 
parse_p parse_quat( parse_p P, float4* q ) {

	parse_float3( P, q );

  float w = 1.0 - (q->x*q->x) - (q->y*q->y) - (q->z*q->z);
  if( w < 0.0f )
    q->w = 0.0f;
  else
    q->w = -sqrt(w);

  return P;

}

static 
md5model_joint_p parse_joints( parse_p P, md5model_p mdl ) {

	if( !parsok( match( ff(P), "joints" ) ) ) return NULL;
	if( !parsok( matchc( ff(P), '{' ) ) ) return NULL;

	md5model_joint_p joints = new_array(mdl, md5model_joint_t, mdl->n_joints );
	for( int i=0; i<mdl->n_joints; i++ ) {
		
		md5model_joint_p joint = &joints[i];
		int parent_idx;

		qstring( ff(P), joints, &joint->name );
		integer( ff(P), &parent_idx );
		if( parent_idx >= 0 )
			joint->parent = &joints[parent_idx];

		parse_float3( ff(P), &joint->pos ); joint->pos.w = 1.0f;
		parse_quat( ff(P), &joint->orient );

		if( !parsok(P) ) {
			delete( joints );
			return NULL;
		}

	}

	if( !parsok( matchc( ff(P), '}' ) ) ) {
		delete(joints);
		return NULL;
	}

	return joints;

}

static
parse_p parse_texcoord( parse_p P, float* s, float* t ) {
	
	matchc( ff(P), '(' );
	decimalf( ff(P), s );
	decimalf( ff(P), t );
	matchc( ff(P), ')' );
	
	return P;
	
}

static
parse_p parse_mesh( parse_p P, const void* pool, md5model_joint_p joints, md5model_mesh_p mesh ) {

		qstring( ff( match( ff(P), "shader" ) ), pool,   &mesh->shader );
		integer( ff( match( ff(P), "numverts" ) ), &mesh->n_verts );
		if( !parsok(P) ) 
			return P;

		mesh->verts = new_array( pool, md5model_vert_t, mesh->n_verts );
		for( int i=0; i<mesh->n_verts; i++ ) {

			int idx; 

			match( ff(P), "vert" );
			integer( ff(P), &idx );
			parse_texcoord( ff(P), &mesh->verts[idx].s, &mesh->verts[idx].t );
			integer( ff(P), &mesh->verts[idx].start_weight );
			integer( ff(P), &mesh->verts[idx].n_weights );

			if( !parsok(P) ) 
				return P;

		}
		
		integer( ff( match( ff(P), "numtris" ) ), &mesh->n_tris );
		if( !parsok(P) ) 
			return P;

		mesh->tris = new_array( pool, int, 3 * mesh->n_tris );
		for( int i=0; i<mesh->n_tris; i++ ) {

			int idx; 

			match( ff(P), "tri" );
			integer( ff(P), &idx );
			integer( ff(P), &mesh->tris[3*idx + 0] );
			integer( ff(P), &mesh->tris[3*idx + 1] );
			integer( ff(P), &mesh->tris[3*idx + 2] );

			if( !parsok(P) ) 
				return P;

		}

		integer( ff( match( ff(P), "numweights" ) ), &mesh->n_weights );
		if( !parsok(P) ) 
			return P;

		mesh->weights = new_array( pool, md5model_weight_t, mesh->n_weights );
		for( int i=0; i<mesh->n_weights; i++ ) {

			int idx, joint;

			match( ff(P), "weight" );
			integer( ff(P), &idx );
			integer( ff(P), &joint ); mesh->weights[i].joint = &joints[joint];
			decimalf( ff(P), &mesh->weights[i].bias );
			parse_float3( ff(P), &mesh->weights[i].pos ); mesh->weights[i].pos.w = 1.0f;

		}

		return P;
}

static
md5model_mesh_p parse_meshes( parse_p P, md5model_p mdl ) {

	md5model_mesh_p meshes = new_array(mdl, md5model_mesh_t, mdl->n_meshes );
	for( int i=0; i<mdl->n_meshes; i++ ) {

		match( ff(P), "mesh" );
		matchc( ff(P), '{' );

		md5model_mesh_p mesh = &meshes[i];

		parse_mesh( P, meshes, mdl->joints, mesh );
		if( !parsok( matchc( ff(P), '}' ) ) ) {
			delete(meshes);
			return NULL;
		}

	}

	return meshes;

}

static
md5model_p parse_md5_model( parse_p P ) {

	md5model_p mdl = parse_header(P);

	if( mdl ) {
		mdl->joints = parse_joints(P, mdl);
		mdl->meshes = parse_meshes(P, mdl);
	}

	return mdl;

}

// Public API /////////////////////////////////////////////////////////////////

resource_p load_resource_MD5( int sz, const void* data ) {

	parse_p P = new_buf_PARSE( sz, (const char*)data );

	// Parse the structure
	md5model_p mdl = parse_md5_model( P );

	if( !parsok(P) ) {

		delete(mdl);
		delete(P);

		return NULL;

	}

	// Compute its size
	int size = sizeof(md5model_t);
	
	for( int i=0; i<mdl->n_joints; i++ ) {

		md5model_joint_p joint = &mdl->joints[i];
		size = size + sizeof(md5model_joint_t)
			+ strlen(joint->name) + 1;

	}

	for( int i=0; i<mdl->n_meshes; i++ ) {

		md5model_mesh_p mesh = &mdl->meshes[i];
		size = size + sizeof(md5model_mesh_t)
			+ strlen(mesh->shader) + 1
			+ mesh->n_verts * sizeof(md5model_vert_t)
			+ 3 * mesh->n_tris * sizeof(mesh->tris[0])
			+ mesh->n_weights * sizeof(md5model_weight_t);

	}
		
	// Create the resource
	return create_raw_RES( sz, mdl, -1 );

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

	md5model_p mdl = resource(md5model_t,argv[1]);
	if( !mdl ) {
		printf("Failed to load: %s\n", argv[1]);
		return 255;
	}

	printf("md5stat %s:\n\n", argv[1]);
	printf("version:\t%d\n", mdl->version);
	printf("n_joints:\t%d\n", mdl->n_joints);
	printf("n_meshes:\t%d\n", mdl->n_meshes);

	printf("joints:\n");
	for( int i=0; i<mdl->n_joints; i++ ) {

		md5model_joint_p joint = &mdl->joints[i];	
		printf("\t% 3d: %s (%s)\n", i, joint->name, joint->parent ? joint->parent->name : "none");

	}

	printf("meshes:\n");
	for( int i=0; i<mdl->n_meshes; i++ ) {

		md5model_mesh_p mesh = &mdl->meshes[i];	
		printf("\t% 3d: verts: % 4d\ttris: % 4d\tweights: % 4d\n", i, mesh->n_verts, mesh->n_tris, mesh->n_weights);

	}

	return 0;
}

#endif
