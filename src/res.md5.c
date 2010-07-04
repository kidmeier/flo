#include "core.alloc.h"
#include "parse.core.h"
#include "res.core.h"
#include "res.md5.h"
#include "math.vec.h"

static 
md5model_p parse_header( parse_p P ) {

	if( !parsok( match( skipws(P), "MD5Version") ) )
		return NULL;

	int version; integer( P, &version );
	if( !parsok(P) || 10 != version ) return NULL;
	if( !parsok( match( skipws(P), "commandline" ) ) ) return NULL;
	if( !parsok( qstring( P, NULL, NULL ) ) ) return NULL;
	if( !parsok( match( skipws(P), "numJoints" ) ) ) return NULL;
	int n_joints; integer( P, &n_joints );
	if( !parsok(P) ) return NULL;
	int n_meshes; integer( P, &n_meshes );
	if( !parsok(P) ) return NULL;

	md5model_p mdl = new( NULL, md5model_t );

	mdl->version = version;
	mdl->n_joints = n_joints;
	mdl->n_meshes = n_meshes;

	return mdl;

}

static 
parse_p parse_float3( parse_p P, float4* pos) {
	
	matchc( skipws(P), '(' );
	decimalf( skipws(P), &pos->x );
	decimalf( skipws(P), &pos->y );
	decimalf( skipws(P), &pos->z ); 
	matchc( skipws(P), ')' ); 

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

	if( !parsok( match( skipws(P), "joints" ) ) ) return NULL;
	if( !parsok( matchc( skipws(P), '{' ) ) ) return NULL;

	md5model_joint_p joints = new_array(mdl, md5model_joint_t, mdl->n_joints );
	for( int i=0; i<mdl->n_joints; i++ ) {
		
		md5model_joint_p joint = &joints[i];
		int parent_idx;

		qstring( skipws(P), joints, &joint->name );
		integer( skipws(P), &parent_idx );
		parse_float3( skipws(P), &joint->pos ); joint->pos.w = 1.0f;
		parse_quat( skipws(P), &joint->orient );

		if( !parsok(P) ) {
			delete( joints );
			return NULL;
		}

	}

	if( !parsok( matchc( skipws(P), '}' ) ) ) {
		delete(joints);
		return NULL;
	}

	return joints;

}

static
parse_p parse_texcoord( parse_p P, float* s, float* t ) {
	
	matchc( skipws(P), '(' );
	decimalf( skipws(P), s );
	decimalf( skipws(P), t );
	matchc( skipws(P), ')' );
	
	return P;
	
}

static
parse_p parse_mesh( parse_p P, const void* pool, md5model_joint_p joints, md5model_mesh_p mesh ) {

		qstring( skipws( match( skipws(P), "shader" ) ), pool,   &mesh->shader );
		integer( skipws( match( skipws(P), "numverts" ) ), &mesh->n_verts );
		if( !parsok(P) ) 
			return P;

		mesh->verts = new_array( pool, md5model_vert_t, mesh->n_verts );
		for( int i=0; i<mesh->n_verts; i++ ) {

			int idx; 

			match( skipws(P), "vert" );
			integer( skipws(P), &idx );
			parse_texcoord( skipws(P), &mesh->verts[idx].s, &mesh->verts[idx].t );
			integer( skipws(P), &mesh->verts[idx].start_weight );
			integer( skipws(P), &mesh->verts[idx].n_weights );

			if( !parsok(P) ) 
				return P;

		}
		
		integer( skipws( match( skipws(P), "numtris" ) ), &mesh->n_tris );
		if( !parsok(P) ) 
			return P;

		mesh->tris = new_array( pool, int, 3 * mesh->n_tris );
		for( int i=0; i<mesh->n_tris; i++ ) {

			int idx; 

			match( skipws(P), "tri" );
			integer( skipws(P), &idx );
			integer( skipws(P), &mesh->tris[3*idx + 0] );
			integer( skipws(P), &mesh->tris[3*idx + 1] );
			integer( skipws(P), &mesh->tris[3*idx + 2] );

			if( !parsok(P) ) 
				return P;

		}

		integer( skipws( match( skipws(P), "numtris" ) ), &mesh->n_weights );
		if( !parsok(P) ) 
			return P;

		mesh->weights = new_array( pool, md5model_weight_t, mesh->n_weights );
		for( int i=0; i<mesh->n_weights; i++ ) {

			int idx, joint;

			match( skipws(P), "weight" );
			integer( skipws(P), &idx );
			integer( skipws(P), &joint ); mesh->weights[i].joint = &joints[joint];
			decimalf( skipws(P), &mesh->weights[i].bias );
			parse_float3( skipws(P), &mesh->weights[i].pos ); mesh->weights[i].pos.w = 1.0f;

		}

		return P;
}

static
md5model_mesh_p parse_meshes( parse_p P, md5model_p mdl ) {

	md5model_mesh_p meshes = new_array(mdl, md5model_mesh_t, mdl->n_meshes );
	for( int i=0; i<mdl->n_meshes; i++ ) {

		match( skipws(P), "mesh" );
		matchc( skipws(P), '{' );

		md5model_mesh_p mesh = &meshes[i];

		parse_mesh( P, meshes, mdl->joints, mesh );
		if( !parsok( matchc( skipws(P), '}' ) ) ) {
			delete(meshes);
			return NULL;
		}

	}

	return meshes;

}

static
md5model_p parse_md5_model( parse_p P ) {

	md5model_p mdl = parse_header(P);
	
	mdl->joints = parse_joints(P, mdl);
	mdl->meshes = parse_meshes(P, mdl);

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

int main( int argc, char* argv[] ) {

	

}

#endif
