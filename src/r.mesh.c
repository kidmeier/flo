#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "control.minmax.h"
#include "core.types.h"
#include "gl.attrib.h"
#include "gl.index.h"
#include "res.core.h"
#include "res.io.h"
#include "r.mesh.h"

void         write_Mesh( pointer res, FILE *outp ) {

	Mesh *mesh = (Mesh*)res;

	write_Res_uint32( outp, (uint32_t)mesh->n_verts  );
	write_Res_uint32( outp, (uint32_t)mesh->n_uvs    );
	write_Res_uint32( outp, (uint32_t)mesh->n_normals);
	write_Res_uint32( outp, (uint32_t)mesh->n_tris   );

	write_Res_buf( outp, 3 * sizeof(mesh->verts[0]  ) * mesh->n_verts  , mesh->verts   );
	write_Res_buf( outp, 2 * sizeof(mesh->uvs[0]    ) * mesh->n_uvs    , mesh->uvs     );
	write_Res_buf( outp, 3 * sizeof(mesh->normals[0]) * mesh->n_normals, mesh->normals );
	write_Res_buf( outp, 3 * sizeof(mesh->tris[0]   ) * mesh->n_tris   , mesh->tris    );

}

pointer      *read_Mesh( FILE *inp ) {

	uint32_t vertc, uvc, normalc, tric;

	// Read the extents
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

	return (pointer)mesh;

}

Drawable *drawable_Mesh( region_p R, Mesh *mesh ) {

	Vattrib *verts   = new_Vattrib( "pos", 3, GL_FLOAT, GL_FALSE );
	Vattrib *normals = (NULL == verts  ) ? NULL : new_Vattrib( "n", 3, GL_FLOAT, GL_FALSE );
	Vattrib *texcs   = (NULL == normals) ? NULL : new_Vattrib( "uv", 2, GL_FLOAT, GL_FALSE );
	Vindex *tris     = (NULL == texcs  ) ? NULL : new_Vindex( GL_UNSIGNED_INT );
	
	float *vp = alloc_Vattrib( verts, staticDraw, mesh->n_verts );
	float *np = alloc_Vattrib( normals, staticDraw, mesh->n_normals );
	float *tp = alloc_Vattrib( texcs, staticDraw, max( 1, mesh->n_uvs ) );
	uint  *trip = alloc_Vindex( tris, staticDraw, 3 * mesh->n_tris );

	if( !vp || !np || !tp || !trip ) {

		delete_Vattrib(verts);
		delete_Vattrib(texcs);
		delete_Vattrib(normals);
		delete_Vindex(tris);

		return NULL;

	}

	memcpy( vp, mesh->verts  , 3 * sizeof(float) * mesh->n_verts   );
	if( mesh->n_uvs > 0 )
		memcpy( tp, mesh->uvs, 2 * sizeof(float) * mesh->n_uvs     );
	else
		memset( tp, 0, 2 * sizeof(float) * 1 );
	memcpy( np, mesh->normals, 3 * sizeof(float) * mesh->n_normals );
	memcpy( trip, mesh->tris , 3 * sizeof(uint)  * mesh->n_tris    );

	flush_Vattrib( verts );
	flush_Vattrib( texcs );
	flush_Vattrib( normals );
	flush_Vindex( tris );
	
	return new_Drawable_indexed( R, 3 * mesh->n_tris, tris,
	                             define_Varray( 3, verts, texcs, normals ),
	                             drawTris );

}
