#ifndef __res_md5_h__
#define __res_md5_h__

#include "math.vec.h"

struct md5model_joint_s {

  char* name;
  
  struct md5model_joint_s* parent;
  float4 pos;
  float4 orient;

};
typedef struct md5model_joint_s md5model_joint_t;
typedef md5model_joint_t* md5model_joint_p;

struct md5model_weight_s {

  md5model_joint_p joint;

  float  bias;
	float4 pos;

};
typedef struct md5model_weight_s md5model_weight_t;
typedef md5model_weight_t* md5model_weight_p;

struct md5model_vert_s {

  float s, t;
  
	int               start_weight;
  int               n_weights;
  md5model_weight_p weights;

};
typedef struct md5model_vert_s md5model_vert_t;
typedef md5model_vert_t* md5model_vert_p;

struct md5model_mesh_s {

  char* shader;
  
  int               n_verts;
  md5model_vert_p   verts;

  int               n_tris;
	int*              tris;

	int               n_weights;
	md5model_weight_p weights;

};
typedef struct md5model_mesh_s md5model_mesh_t;
typedef md5model_mesh_t* md5model_mesh_p;

struct md5model_s {

  int version;

  int n_joints;
  int n_meshes;

  md5model_mesh_p   meshes;
  md5model_joint_p  joints;

};
typedef struct md5model_s md5model_t;
typedef md5model_t* md5model_p;


#endif
