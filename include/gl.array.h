#ifndef __gl_array_h__
#define __gl_array_h__

#include "gl.attrib.h"

// Vertex arrays ////////////////////////////////////////////////////////////

typedef struct varray_s  varray_t;
typedef struct varray_s* varray_p;

varray_p define_VARRAY(int n, ...);
varray_p new_VARRAY( int n, vattrib_p vattribs[] );
void     delete_VARRAY( varray_p varray );
varray_p bind_VARRAY( varray_p varray );

#endif
