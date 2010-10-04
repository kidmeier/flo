#ifndef __gl_array_h__
#define __gl_array_h__

#include "gl.attrib.h"

// Vertex arrays ////////////////////////////////////////////////////////////

typedef struct Varray Varray;

Varray* define_Varray(int n, ...);
Varray*    new_Varray( int n, Vattrib* vattribs[] );
void    delete_Varray( Varray* varray );
Varray*   bind_Varray( Varray* varray );

#endif
