#ifndef __res_md5_h__
#define __res_md5_h__

#include "math.vec.h"

struct md5mesh_joint_s {

  char* name;
  
  struct md5mesh_joint_s* parent;
  vec3_t pos;
  quat_t orient;

};
typedef struct md5mesh_joint_s md5mesh_joint_t;
typedef md5mesh_joint_t* md5mesh_joint_p;

struct md5mesh_weight_s {

  md5mesh_joint_p joint;

  float  bias;
  vec3_t pos;

};
typedef struct md5mesh_weight_s md5mesh_weight_t;
typedef md5mesh_weight_t* md5mesh_weight_p;

struct md5mesh_vert_s {

  float s, t;
  
  int              n_weights;
  md5mesh_weight_p weights;

};
typedef struct md5mesh_vert_s md5mesh_vert_t;
typedef md5mesh_vert_t* md5mesh_vert_p;

struct md5mesh_mesh_s {

  char* shader;
  
  int              n_verts;
  md5mesh_vert_p   verts;

  int              n_tris;
  short*           tris;

  int              n_weights;
  md5mesh_weight_p weights;

};
typedef struct md5mesh_mesh_s md5mesh_mesh_t;
typedef md5mesh_mesh_t* md5mesh_mesh_p;

struct md5mesh_s {

  int version;
  const char* command;

  int n_joints;
  int n_meshes;

  md5mesh_mesh_p   meshes;
  md5mesh_joint_p  joints;

};
typedef struct md5mesh_s md5mesh_t;
typedef md5mesh_t* md5mesh_p;

md5mesh_p parse_mesh_MD5( const char* path );

#endif
