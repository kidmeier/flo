#include <assert.h>
#include <string.h>

#include "control.maybe.h"
#include "core.log.h"
#include "r.skel.h"
#include "res.io.h"

#define r_skelVersion 1

static void write_verts( FILE *outp, Skel_Mesh *mesh ) {

	for( int i=0; i<mesh->n_verts; i++ ) {

		Skel_Vertex *vert = &mesh->verts[i];

		write_Res_float( outp, vert->s );
		write_Res_float( outp, vert->t );

		uint32_t first_weight = vert->weights - mesh->weights;

		write_Res_uint32( outp, first_weight );
		write_Res_uint32( outp, vert->count );

	}

}

static void read_verts( FILE *inp, Skel_Mesh *mesh ) {

	for( int i=0; i<mesh->n_verts; i++ ) {

		Skel_Vertex *vert = &mesh->verts[i];

		read_Res_float( inp, &vert->s );
		read_Res_float( inp, &vert->t );

		uint32_t first_weight; 

		read_Res_uint32( inp, &first_weight );
		read_Res_uint32( inp, &vert->count );

		assert( first_weight < mesh->n_weights );
		assert( first_weight + vert->count <= mesh->n_weights );

		vert->weights = &mesh->weights[ first_weight ];

	}

}

static void write_weights( FILE *outp, Skeleton *skel, Skel_Mesh *mesh ) {

	for( int i=0; i<mesh->n_weights; i++ ) {

		Skel_Weight *weight = &mesh->weights[i];

		uint32_t joint = weight->joint - skel->joints;
		
		write_Res_uint32( outp, joint );
		write_Res_float( outp, weight->bias );
		write_Res_float4( outp, weight->pos );

	}

}

static void read_weights( FILE *inp, Skeleton *skel, Skel_Mesh *mesh ) {

	for( int i=0; i<mesh->n_weights; i++ ) {

		Skel_Weight *weight = &mesh->weights[i];

		uint32_t joint;
		
		read_Res_uint32( inp, &joint );
		read_Res_float( inp, &weight->bias );
		read_Res_float4( inp, &weight->pos );

		assert( joint < skel->n_joints );

		weight->joint = &skel->joints[ joint ];

	}

}

static void write_tris( FILE *outp, Skel_Mesh *mesh ) {

	for( int i=0; i<mesh->n_tris; i++ ) {

		write_Res_uint32( outp, mesh->tris[ 3*i + 0 ] );
		write_Res_uint32( outp, mesh->tris[ 3*i + 1 ] );
		write_Res_uint32( outp, mesh->tris[ 3*i + 2 ] );

	}

}

static void read_tris( FILE *inp, Skel_Mesh *mesh ) {

	for( int i=0; i<mesh->n_tris; i++ ) {

		read_Res_uint32( inp, &mesh->tris[ 3*i + 0 ] );
		read_Res_uint32( inp, &mesh->tris[ 3*i + 1 ] );
		read_Res_uint32( inp, &mesh->tris[ 3*i + 2 ] );

	}

}

void         write_Skel( pointer res, FILE *outp ) {

	Skeleton *skel = (Skeleton*)res;

	write_Res_uint32( outp, r_skelVersion );

	write_Res_uint32( outp, (uint32_t)skel->n_joints );
	write_Res_uint32( outp, (uint32_t)skel->n_meshes );

	for( int i=0; i<skel->n_joints; i++ ) {

		Skel_Joint *joint = &skel->joints[i];
		int32_t    parent = joint->parent ? (joint->parent - &skel->joints[0]) : -1;

		write_Res_string( outp, joint->name );
		write_Res_int32( outp, parent );
		write_Res_float4( outp, joint->p );
		write_Res_float4( outp, joint->qr );

	}

	for( int i=0; i<skel->n_meshes; i++ ) {

		Skel_Mesh *mesh = &skel->meshes[i];

		write_Res_string( outp, mesh->shader );

		write_Res_uint32( outp, mesh->n_verts );
		write_Res_uint32( outp, mesh->n_weights );
		write_Res_uint32( outp, mesh->n_tris );

		write_verts( outp, mesh );
		write_weights( outp, skel, mesh );
		write_tris( outp, mesh );

	}

}

pointer      *read_Skel( FILE *inp ) {

	uint32_t version; read_Res_uint32( inp, &version );

	if( version != r_skelVersion ) {

		error("Version mis-match: expected %u, found %u", r_skelVersion, version );
		return NULL;

	}

	Skeleton *skel = malloc( sizeof(Skeleton) );

	read_Res_uint32( inp, &skel->n_joints );
	read_Res_uint32( inp, &skel->n_meshes );

	skel->joints = calloc( skel->n_joints, sizeof(Skel_Joint) );
	skel->meshes = calloc( skel->n_meshes, sizeof(Skel_Mesh) );

	for( int i=0; i<skel->n_joints; i++ ) {

		Skel_Joint *joint = &skel->joints[i];
		int32_t    parent;

		read_Res_string( inp, (char**)&joint->name );
		read_Res_int32( inp, &parent );
		read_Res_float4( inp, &joint->p );
		read_Res_float4( inp, &joint->qr );

		assert( (parent < 0) ? true : (parent < skel->n_joints) );

		joint->parent = parent < 0 ? NULL : &skel->joints[ parent ];

	}

	for( int i=0; i<skel->n_meshes; i++ ) {

		Skel_Mesh *mesh = &skel->meshes[i];

		read_Res_string( inp, (char**)&mesh->shader );

		read_Res_uint32( inp, &mesh->n_verts );
		read_Res_uint32( inp, &mesh->n_weights );
		read_Res_uint32( inp, &mesh->n_tris );

		mesh->verts = calloc( mesh->n_verts, sizeof(Skel_Vertex) );
		mesh->weights = calloc( mesh->n_weights, sizeof(Skel_Weight) );
		mesh->tris = calloc( sizeof(uint32_t), 3*mesh->n_tris );

		read_verts( inp, mesh );
		read_weights( inp, skel, mesh );
		read_tris( inp, mesh );

	}

	info0( "Loaded skeleton:" );
	dump_Skel_info( skel );

	return (pointer)skel;

}

void          dump_Skel_info( Skeleton *skel ) {

	info( "\t# joints : %u", skel->n_joints );
	for( int i=0; i<skel->n_joints; i++ ) {

		Skel_Joint *joint = &skel->joints[i];

		info("\tjoint #%u :", i);
		info("\t\tname   : %s", joint->name);
		info("\t\tparent : %s", joint->parent ? joint->parent->name : "none");

	}

	info( "\t# meshes : %u", skel->n_meshes );
	for( int i=0; i<skel->n_meshes; i++ ) {

		Skel_Mesh *mesh = &skel->meshes[i];

		info( "\tmesh #%u    :", i );
		info( "\t\tshader    : %s", mesh->shader );
		info( "\t\tn_verts   : %u", mesh->n_verts );
		info( "\t\tn_weights : %u", mesh->n_weights );
		info( "\t\tn_tris    : %u", mesh->n_tris );

	}

}

Drawable* drawable_Skel( region_p R, Skeleton *skel, int which_mesh ) {

	// Attribs:
	//  0 pos:    x, y, z
	//  1 uv:     s, t
	//  2 normal: nx, ny, nz
	Vattrib* verts   = new_Vattrib( "pos",       3, GL_FLOAT, GL_FALSE );
	Vattrib* texcs   = maybe( verts, == NULL, 
	                          new_Vattrib( "uv", 2, GL_FLOAT, GL_FALSE ) );
	Vattrib* normals = maybe( texcs, == NULL, 
	                          new_Vattrib( "N",  3, GL_FLOAT, GL_FALSE ) );
	Vindex*  tris    = maybe( (Vindex*)normals, == NULL, 
	                          new_Vindex( GL_UNSIGNED_INT ) );

	if( !tris ) {

		maybe( verts, == NULL, delete_Vattrib(verts) );
		maybe( texcs, == NULL, delete_Vattrib(texcs) );
		maybe( normals, == NULL, delete_Vattrib(normals) );
		maybe( tris, == NULL, delete_Vindex(tris) );
		return NULL;

	}

	Skel_Mesh *mesh = &skel->meshes[which_mesh];

	float*  vp = alloc_Vattrib( verts, staticDraw, mesh->n_verts );
	float*  tp = maybe( vp, == NULL, 
	                    alloc_Vattrib( texcs, staticDraw, mesh->n_verts ) );
	float*  np = maybe( tp, == NULL, 
	                    alloc_Vattrib( normals, staticDraw, mesh->n_verts ) );
	uint* trip = maybe( (uint*)np, == NULL, 
	                    alloc_Vindex( tris, staticDraw, 3 * mesh->n_tris ) );
	
	if( !vp || !tp || !np || !trip ) {

		delete_Vattrib(verts);
		delete_Vattrib(texcs);
		delete_Vattrib(normals);
		delete_Vindex(tris);

		return NULL;
	}

	// Compute vertex positions
	for( int i=0; i<mesh->n_verts; i++ ) {

		Skel_Vertex *vert = &mesh->verts[i];
		float4        pos = { 0.f, 0.f, 0.f, 0.f };

		// Write tex coords
		tp[ 2*i + 0 ] = vert->s;
		tp[ 2*i + 1 ] = vert->t;

		// Add up the weights
		for( int j=0; j<vert->count; j++ ) {

			Skel_Weight *wgt = &vert->weights[ j ];
			Skel_Joint  *joint = wgt->joint;
			
			float4 wv = qrot( joint->qr, wgt->pos );
			pos = vadd( pos, vscale(wgt->bias, vadd( joint->p, wv)) );
			
		}

		// Write pos
		vp[ 3*i + 0 ] = pos.x;
		vp[ 3*i + 1 ] = pos.y;
		vp[ 3*i + 2 ] = pos.z;
	   
	}

	// To compute the normal at each vertex, we compute the normal for each tri
	// and sum the result on the normal at each vertex. Then we normalize and
	// presto-chango, we have the average normal at each vertex.
	//
	// So to start, we need to clear the normals to 0
	memset( np, 0, 3*sizeof(float)*mesh->n_verts );

	// Compute normals for each face
	for( int i=0; i<mesh->n_tris; i++ ) {

		// Copy indices
		trip[ 3*i + 0 ] = mesh->tris[ 3*i + 0 ];
		trip[ 3*i + 1 ] = mesh->tris[ 3*i + 1 ];
		trip[ 3*i + 2 ] = mesh->tris[ 3*i + 2 ];

		// Compute normal
		float4 a = { vp[trip[3*i + 0] + 0], vp[trip[3*i + 0] + 1], vp[trip[3*i + 0] + 2], 1.f };
		float4 b = { vp[trip[3*i + 1] + 0], vp[trip[3*i + 1] + 1], vp[trip[3*i + 1] + 2], 1.f };
		float4 c = { vp[trip[3*i + 1] + 0], vp[trip[3*i + 2] + 1], vp[trip[3*i + 2] + 2], 1.f };
		float4 n = vnormal( vcross( vsub( c, a ), vsub( c, b ) ) );
		
		// Copy normal; this looks complicated but heres the breakdown:
		//  Each triangle is a triple of indices into the verts/normals array
		//  So reference the vertex index via `trip`, then index that into
		//  the normal array, and store the data.
		//
		//  Do this for each vertex of the triangle.
		np[ 3*trip[3*i + 0] + 0 ] += n.x;
		np[ 3*trip[3*i + 0] + 1 ] += n.y;
		np[ 3*trip[3*i + 0] + 2 ] += n.z;

		np[ 3*trip[3*i + 1] + 0 ] += n.x;
		np[ 3*trip[3*i + 1] + 1 ] += n.y;
		np[ 3*trip[3*i + 1] + 2 ] += n.z;

		np[ 3*trip[3*i + 2] + 0 ] += n.x;
		np[ 3*trip[3*i + 2] + 1 ] += n.y;
		np[ 3*trip[3*i + 2] + 2 ] += n.z;

	}

	// Now go through and normalize each normal
	for( int i=0; i<mesh->n_verts; i++ ) {

		float4 N = vnormal((float4){ np[ 3*i + 0 ], np[ 3*i + 1 ], np[ 3*i + 2 ], 0.0f });
		np[ 3*i + 0 ] = N.x;
		np[ 3*i + 1 ] = N.y;
		np[ 3*i + 2 ] = N.z;

	}

	// Flush to VRAM
	flush_Vattrib( verts );
	flush_Vattrib( texcs );
	flush_Vattrib( normals );
	flush_Vindex( tris );

	// Package it all up
	return new_Drawable_indexed( R,
	                             3 * mesh->n_tris,
	                             tris, 
	                             define_Varray( 3, verts, texcs, normals ),
	                             drawTris );

}
