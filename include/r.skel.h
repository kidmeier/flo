#ifndef __r_skel_H__
#define __r_skel_H__

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
	
	int        start_weight;
	int        n_weights;

	Skel_Weight *weights;
	
};


typedef struct Skel_Mesh Skel_Mesh;
struct Skel_Mesh {
	
	const char* shader;
	
	int        n_verts;
	int        n_weights;
	int        n_tris;

	Skel_Vertex *verts;
	Skel_Weight *weights;
	int         *tris;	
	
};

typedef struct Skeleton Skeleton;
struct Skeleton {
	
	int n_joints;
	int n_meshes;
	
	Skel_Mesh  *meshes;
	Skel_Joint *joints;
	
};

Drawable* drawable_Skel( region_p R, Skeleton *skel, int which_mesh );

#endif
