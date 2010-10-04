#ifndef __gl_attrib_h__
#define __gl_attrib_h__

#include "gl.buf.h"

// Vertex attribute buffers ///////////////////////////////////////////////////
typedef struct Vattrib  Vattrib;

Vattrib*  new_Vattrib( const char* name, uint width );
void   delete_Vattrib( Vattrib* vattrib );

pointer alloc_Vattrib( Vattrib* vattrib, uint n, GLenum usage );
pointer   map_Vattrib( Vattrib* vattrib, GLenum access );
pointer   map_Vattrib_range( Vattrib* vattrib, GLbitfield access, uint ofs, uint n );
void    flush_Vattrib( Vattrib* vattrib );
void    flush_Vattrib_range( Vattrib* vattrib, uint ofs, uint n );

int    upload_Vattrib( Vattrib* vattrib, GLenum usage, uint n, pointer data );
int    upload_Vattrib_range( Vattrib* vattrib, uint ofs, uint n, pointer data );

#endif
