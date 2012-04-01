#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "control.minmax.h"
#include "core.log.h"
#include "core.types.h"
#include "gl.attrib.h"
#include "gl.index.h"
#include "res.core.h"
#include "res.io.h"
#include "r.mesh.h"

#define r_meshVersion 3

void         write_Mesh( pointer res, FILE *outp ) {

	Mesh *mesh = (Mesh*)res;

	write_Res_uint32( outp, r_meshVersion );

	write_Res_uint32( outp, (uint32_t)mesh->n_verts  );
	write_Res_uint32( outp, (uint32_t)mesh->n_uvs    );
	write_Res_uint32( outp, (uint32_t)mesh->n_normals);
	write_Res_uint32( outp, (uint32_t)mesh->n_tris   );

	write_Res_buf( outp, 3 * sizeof(mesh->verts[0]  ) * mesh->n_verts  , mesh->verts   );
	write_Res_buf( outp, 2 * sizeof(mesh->uvs[0]    ) * mesh->n_uvs    , mesh->uvs     );
	write_Res_buf( outp, 3 * sizeof(mesh->normals[0]) * mesh->n_normals, mesh->normals );
	write_Res_buf( outp, 3 * sizeof(mesh->tris[0]   ) * mesh->n_tris   , mesh->tris    );

	write_Res_float4( outp, mesh->bounds.mins );
	write_Res_float4( outp, mesh->bounds.maxs );

}

pointer      *read_Mesh( FILE *inp ) {

	uint32_t version;

	read_Res_uint32( inp, &version );
	if( version != r_meshVersion ) {

		error("Version mis-match: expected %u, found %u", r_meshVersion, version );
		return NULL;

	}

	// Read the extents
	uint32_t vertc, uvc, normalc, tric;

	read_Res_uint32( inp, &vertc );
	read_Res_uint32( inp, &uvc );
	read_Res_uint32( inp, &normalc );
	read_Res_uint32( inp, &tric );

	Mesh *mesh = malloc( sizeof(Mesh) );

	mesh->n_verts   = vertc;
	mesh->n_uvs     = uvc;
	mesh->n_normals = normalc;
	mesh->n_tris    = tric;

	mesh->verts   = malloc( 3 * vertc * sizeof(mesh->verts[0]) );
	mesh->uvs     = malloc( 2 * uvc * sizeof(mesh->uvs[0]) );
	mesh->normals = malloc( 3 * normalc * sizeof(mesh->normals[0]) );
	mesh->tris    = malloc( 3 * tric * sizeof(mesh->tris[0]) );

	read_Res_buf( inp, 3 * sizeof(mesh->verts[0])   * vertc  , mesh->verts   );
	read_Res_buf( inp, 2 * sizeof(mesh->uvs[0])     * uvc    , mesh->uvs     );
	read_Res_buf( inp, 3 * sizeof(mesh->normals[0]) * normalc, mesh->normals );
	read_Res_buf( inp, 3 * sizeof(mesh->tris[0])    * tric   , mesh->tris    );

	read_Res_float4( inp, &mesh->bounds.mins );
	read_Res_float4( inp, &mesh->bounds.maxs );

	info0( "Loaded mesh:" );
	dump_Mesh_info( mesh );

	return (pointer)mesh;

}

void          dump_Mesh_info( Mesh *mesh ) {

	info( "\tvertices :  %d", mesh->n_verts );
	info( "\ttexcoords:  %d", mesh->n_uvs );
	info( "\tnormals  :  %d", mesh->n_normals );
	info( "\tfaces    :  %d", mesh->n_tris );
	info( "\tmins     :  %6.2f %6.2f %6.2f", 
	      mesh->bounds.mins.x, mesh->bounds.mins.y, mesh->bounds.mins.z );
	info( "\tmaxs     :  %6.2f %6.2f %6.2f", 
	      mesh->bounds.maxs.x, mesh->bounds.maxs.y, mesh->bounds.maxs.z );

}

static void compute_normals( float *np, float *verts, int n_tris, Mesh_Vertex *tris ) {

	assert( np );
	assert( verts );
	assert( n_tris > 0 );
	assert( tris );

	for( int i=0; i<n_tris; i++ ) {

		float4 v0, v1, v2;

		v0.x = verts[ 3*tris[3*i + 0].v + 0 ];
		v0.y = verts[ 3*tris[3*i + 0].v + 1 ];
		v0.z = verts[ 3*tris[3*i + 0].v + 2 ];
		v0.w = 1.f;

		v1.x = verts[ 3*tris[3*i + 1].v + 0 ];
		v1.y = verts[ 3*tris[3*i + 1].v + 1 ];
		v1.z = verts[ 3*tris[3*i + 1].v + 2 ];
		v1.w = 1.f;

		v2.x = verts[ 3*tris[3*i + 2].v + 0 ];
		v2.y = verts[ 3*tris[3*i + 2].v + 1 ];
		v2.z = verts[ 3*tris[3*i + 2].v + 2 ];
		v2.w = 1.f;

		float4 u = vsub( v1, v0 );
		float4 v = vsub( v2, v0 );

		float4 n = vnormal( vcross( u, v ) );

		for( int j=0; j<3; j++ ) {
			np[ 3*(3*i + j) + 0 ] = n.x;
			np[ 3*(3*i + j) + 1 ] = n.y;
			np[ 3*(3*i + j) + 2 ] = n.z;
		}

	}

}

static void copy_tris( int n_tris, Mesh_Vertex *tris, 
                       float *dst_v, float *src_v,
                       float *dst_uv, float *src_uv,
                       float *dst_n, float *src_n ) {

	assert( n_tris > 0 );
	assert( tris );
	assert( dst_v ); assert( src_v );
	assert( dst_uv ); assert( src_uv );
	assert( dst_n ); assert( src_n );
	
	for( int i=0; i<n_tris; i++ ) {

		// Verts
		dst_v[ 3*(3*i+0) + 0 ] = src_v[ 3*tris[3*i+0].v + 0 ];
		dst_v[ 3*(3*i+0) + 1 ] = src_v[ 3*tris[3*i+0].v + 1 ];
		dst_v[ 3*(3*i+0) + 2 ] = src_v[ 3*tris[3*i+0].v + 2 ];

		dst_v[ 3*(3*i+1) + 0 ] = src_v[ 3*tris[3*i+1].v + 0 ];
		dst_v[ 3*(3*i+1) + 1 ] = src_v[ 3*tris[3*i+1].v + 1 ];
		dst_v[ 3*(3*i+1) + 2 ] = src_v[ 3*tris[3*i+1].v + 2 ];

		dst_v[ 3*(3*i+2) + 0 ] = src_v[ 3*tris[3*i+2].v + 0 ];
		dst_v[ 3*(3*i+2) + 1 ] = src_v[ 3*tris[3*i+2].v + 1 ];
		dst_v[ 3*(3*i+2) + 2 ] = src_v[ 3*tris[3*i+2].v + 2 ];

		// UVs
		dst_uv[ 2*(3*i+0) + 0 ] = src_uv[ 2*tris[3*i+0].uv + 0 ];
		dst_uv[ 2*(3*i+0) + 1 ] = src_uv[ 2*tris[3*i+0].uv + 1 ];

		dst_uv[ 2*(3*i+1) + 0 ] = src_uv[ 2*tris[3*i+1].uv + 0 ];
		dst_uv[ 2*(3*i+1) + 1 ] = src_uv[ 2*tris[3*i+1].uv + 1 ];

		dst_uv[ 2*(3*i+2) + 0 ] = src_uv[ 2*tris[3*i+2].uv + 0 ];
		dst_uv[ 2*(3*i+2) + 1 ] = src_uv[ 2*tris[3*i+2].uv + 1 ];

		// Normals
		dst_n[ 3*(3*i+0) + 0 ] = src_n[ 3*tris[3*i+0].n + 0 ];
		dst_n[ 3*(3*i+0) + 1 ] = src_n[ 3*tris[3*i+0].n + 1 ];
		dst_n[ 3*(3*i+0) + 2 ] = src_n[ 3*tris[3*i+0].n + 2 ];

		dst_n[ 3*(3*i+1) + 0 ] = src_n[ 3*tris[3*i+1].n + 0 ];
		dst_n[ 3*(3*i+1) + 1 ] = src_n[ 3*tris[3*i+1].n + 1 ];
		dst_n[ 3*(3*i+1) + 2 ] = src_n[ 3*tris[3*i+1].n + 2 ];

		dst_n[ 3*(3*i+2) + 0 ] = src_n[ 3*tris[3*i+2].n + 0 ];
		dst_n[ 3*(3*i+2) + 1 ] = src_n[ 3*tris[3*i+2].n + 1 ];
		dst_n[ 3*(3*i+2) + 2 ] = src_n[ 3*tris[3*i+2].n + 2 ];

	}

}

Drawable *drawable_Mesh( region_p R, Mesh *mesh ) {

	Vattrib *verts   = new_Vattrib( "pos", 3, GL_FLOAT, GL_FALSE );
	Vattrib *normals = (NULL == verts  ) ? NULL : new_Vattrib( "n", 3, GL_FLOAT, GL_FALSE );
	Vattrib *texcs   = (NULL == normals) ? NULL : new_Vattrib( "uv", 2, GL_FLOAT, GL_FALSE );
	
	float *vp = alloc_Vattrib( verts, staticDraw, 3*mesh->n_tris );
	float *np = alloc_Vattrib( normals, staticDraw, 3*mesh->n_tris );
	float *tp = alloc_Vattrib( texcs, staticDraw, 3*mesh->n_tris );

	if( !vp || !np || !tp ) {

		delete_Vattrib(verts);
		delete_Vattrib(texcs);
		delete_Vattrib(normals);

		return NULL;

	}

	copy_tris( mesh->n_tris, mesh->tris, vp, mesh->verts, tp, mesh->uvs, np, mesh->normals );
	if( !(mesh->n_normals > 0) )
		compute_normals( np, mesh->verts, mesh->n_tris, mesh->tris );

	flush_Vattrib( verts );
	flush_Vattrib( texcs );
	flush_Vattrib( normals );

	return new_Drawable( R, 3*mesh->n_tris, define_Varray( 3, verts, texcs, normals ), drawTris );
}
