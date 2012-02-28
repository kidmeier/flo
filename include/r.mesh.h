#ifndef __r_mesh_h__
#define __r_mesh_h__

#include "mm.region.h"
#include "r.drawable.h"

typedef struct Mesh Mesh;
struct Mesh {

	int    n_verts;
	int      n_uvs;
	int  n_normals;

	int     n_tris;

	float   *verts;
	float     *uvs;
	float *normals;

	int      *tris;

};

void         write_Mesh( pointer res, FILE *outp );
pointer      *read_Mesh( FILE *inp );

Drawable *drawable_Mesh( region_p R, Mesh *mesh );

#endif
