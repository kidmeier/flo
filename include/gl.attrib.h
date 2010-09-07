#ifndef __gl_attrib_h__
#define __gl_attrib_h__

#include "gl.buf.h"

// Vertex attribute buffers ///////////////////////////////////////////////////
typedef struct vattrib_s  vattrib_t;
typedef struct vattrib_s* vattrib_p;

vattrib_p new_VATTRIB( const char* name, uint arity, uint mb_sz );
void      delete_VATTRIB( vattrib_p vattrib );

float* alloc_VATTRIB( vattrib_p vattrib, uint n, GLenum usage );
float* map_VATTRIB( vattrib_p vattrib, GLenum access );
float* map_range_VATTRIB( vattrib_p vattrib, GLbitfield access, uint ofs, uint n );
void   flush_VATTRIB( vattrib_p vattrib );
void   flush_range_VATTRIB( vattrib_p vattrib, uint ofs, uint n );

int    upload_VATTRIB( vattrib_p vattrib, GLenum usage, uint n, float* data );
int    upload_range_VATTRIB( vattrib_p vattrib, uint ofs, uint n, float* data );

#endif
