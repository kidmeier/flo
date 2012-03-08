#ifndef __r_skel_H__
#define __r_skel_H__

#include <stdio.h>
#include "math.vec.h"
#include "mm.region.h"
#include "r.drawable.h"

typedef struct Skel_Joint Skel_Joint;
struct Skel_Joint {

	const char *name;
	
	Skel_Joint *parent;

	float4 p;
	float4 qr;

};

typedef struct Skel_Weight Skel_Weight;
struct Skel_Weight {
	
	Skel_Joint * joint;
	
	float  bias;
	float4 pos;
	
};

typedef struct Skel_Vertex Skel_Vertex;
struct Skel_Vertex {
	
	float s, t;
	
	Skel_Weight *weights;
	uint32_t     count;
	
};


typedef struct Skel_Mesh Skel_Mesh;
struct Skel_Mesh {
	
	const char* shader;
	
	uint32_t  n_verts;
	uint32_t  n_weights;
	uint32_t  n_tris;

	Skel_Vertex *verts;
	Skel_Weight *weights;
	uint32_t    *tris;	
	
};

typedef struct Skeleton Skeleton;
struct Skeleton {
	
	uint32_t n_joints;
	uint32_t n_meshes;
	
	Skel_Joint *joints;
	Skel_Mesh  *meshes;
	
};

void         write_Skel( pointer res, FILE *outp );
pointer      *read_Skel( FILE *inp );

void          dump_Skel_info( Skeleton *skel );
Drawable* drawable_Skel( region_p R, Skeleton *skel, int which_mesh );

#endif
