#ifndef __res_obj_h__
#define __res_obj_h__

#include "res.core.h"

typedef struct Obj_res Obj_res;
struct Obj_res {

	int    n_verts;
	int      n_uvs;
	int  n_normals;
	int     n_tris;

	float   *verts;
	float     *uvs;
	float *normals;

	int      *tris;

};

resource_p load_resource_Obj( int sz, const void* data );

#endif
