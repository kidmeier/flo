#ifndef __gl_varray_H__
#define __gl_varray_H__

#include <GL/glew.h>
//
#include "gl.vbo.h"
#include "sys.types.h"

struct varray_s;
typedef struct varray_s* varray_t;

struct varray_s {

  int                   id;

  uint                  size; // # of VBOs
  enum e_vertex_attrib* attribs;
  vbo_t*                vbos;

};

struct mesh_s {

  vbo_t* vbo;
  uint* tris;
  
  topology_t* topology;

};

#endif
