#include <string.h>

#include "control.minmax.h"
#include "core.types.h"
#include "gl.attrib.h"
#include "gl.index.h"
#include "r.obj.h"

Drawable *drawable_Obj( region_p R, Obj_res *obj ) {

	Vattrib *verts   = new_Vattrib( "pos", 3, GL_FLOAT, GL_FALSE );
	Vattrib *normals = (NULL == verts  ) ? NULL : new_Vattrib( "n", 3, GL_FLOAT, GL_FALSE );
	Vattrib *texcs   = (NULL == normals) ? NULL : new_Vattrib( "uv", 2, GL_FLOAT, GL_FALSE );
	Vindex *tris     = (NULL == texcs  ) ? NULL : new_Vindex( GL_UNSIGNED_INT );
	
	float *vp = alloc_Vattrib( verts, staticDraw, obj->n_verts );
	float *np = alloc_Vattrib( normals, staticDraw, obj->n_normals );
	float *tp = alloc_Vattrib( texcs, staticDraw, max( 1, obj->n_uvs ) );
	uint  *trip = alloc_Vindex( tris, staticDraw, 3 * obj->n_tris );

	if( !vp || !np || !tp || !trip ) {

		delete_Vattrib(verts);
		delete_Vattrib(texcs);
		delete_Vattrib(normals);
		delete_Vindex(tris);

		return NULL;

	}

	memcpy( vp, obj->verts  , 3 * sizeof(float) * obj->n_verts   );
	if( obj->n_uvs > 0 )
		memcpy( tp, obj->uvs, 2 * sizeof(float) * obj->n_uvs     );
	else
		memset( tp, 0, 2 * sizeof(float) * 1 );
	memcpy( np, obj->normals, 3 * sizeof(float) * obj->n_normals );
	memcpy( trip, obj->tris , 3 * sizeof(uint)  * obj->n_tris    );

	flush_Vattrib( verts );
	flush_Vattrib( texcs );
	flush_Vattrib( normals );
	flush_Vindex( tris );
	
	return new_Drawable_indexed( R, 3 * obj->n_tris, tris,
	                             define_Varray( 3, verts, texcs, normals ),
	                             drawTris );

}
