#include <assert.h>
#include <string.h>

#include "control.maybe.h"
#include "r.md5.h"

Drawable* drawable_MD5( region_p R, 
                        md5model_p mdl, 
                        int which_mesh ) {

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

	md5model_mesh_p mesh = &mdl->meshes[which_mesh];

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

		md5model_vert_p vert = &mesh->verts[i];
		float4           pos = { 0.f, 0.f, 0.f, 0.f };

		// Write tex coords
		tp[ 2*i + 0 ] = vert->s;
		tp[ 2*i + 1 ] = vert->t;

		// Add up the weights
		for( int j=0; j<vert->n_weights; j++ ) {

			md5model_weight_p  wgt = &mesh->weights[ vert->start_weight + j ];
			md5model_joint_p joint = wgt->joint;
			
			float4 wv = qrot( joint->orient, wgt->pos );
			pos = vadd( pos, vscale(wgt->bias, vadd( joint->pos, wv)) );
			
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
