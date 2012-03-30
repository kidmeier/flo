#ifndef __r_mesh_h__
#define __r_mesh_h__

#include "g.aabb.h"
#include "mm.region.h"
#include "r.drawable.h"

typedef struct Mesh Mesh;
typedef struct Mesh_Vertex Mesh_Vertex;

struct Mesh_Vertex {

	int32_t v;
	int32_t uv;
	int32_t n;

};

struct Mesh {

	int32_t    n_verts;
	int32_t      n_uvs;
	int32_t  n_normals;

	float   *verts;
	float     *uvs;
	float *normals;

	int32_t     n_tris;
	Mesh_Vertex *tris;

	AABB    bounds;

};

void         write_Mesh( pointer res, FILE *outp );
pointer      *read_Mesh( FILE *inp );

void          dump_Mesh_info( Mesh *mesh );
Drawable *drawable_Mesh( region_p R, Mesh *mesh );

#endif
