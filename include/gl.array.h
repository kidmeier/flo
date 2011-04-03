#ifndef __gl_array_h__
#define __gl_array_h__

// Vertex arrays ////////////////////////////////////////////////////////////

#include "gl.attrib.h"
#include "gl.array.h"
#include "gl.index.h"

typedef struct Varray Varray;

Varray* define_Varray( int n, ... );
Varray*    new_Varray( int n, Vattrib* vattribs[] );
Varray*    new_Varray_indexed( int n, Vindex* index, Vattrib* vattribs[] );
void    delete_Varray( Varray* varray );
void      draw_Varray( Varray* varray, 
                       GLenum mode, 
                       GLint first, 
                       GLsizei count );
void      draw_Varray_indexed( Vindex* indices, 
                               Varray* varray, 
                               GLenum mode, 
                               GLsizeiptr first, 
                               GLsizei count );


#endif
