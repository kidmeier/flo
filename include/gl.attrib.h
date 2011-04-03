#ifndef __gl_attrib_h__
#define __gl_attrib_h__

#include "gl.buf.h"

// Vertex attribute buffers ///////////////////////////////////////////////////

typedef struct Vattrib Vattrib;
struct Vattrib {
	
	GLuint      id;
	const char* name;
	
	short       size;
	GLenum      type;
	GLboolean   normalize;
	short       stride;

	pointer     buf;
	
};

Vattrib*  new_Vattrib( const char* name, 
                       short size, 
                       GLenum type, 
                       GLboolean norm );
void   delete_Vattrib( Vattrib* vattrib );

pointer alloc_Vattrib( Vattrib* vattrib, GLenum usage, GLsizeiptr count );
pointer   map_Vattrib( Vattrib* vattrib, GLenum access );
pointer   map_Vattrib_range( Vattrib* vattrib, GLbitfield access, GLintptr ofs, GLsizeiptr n );
void    flush_Vattrib( Vattrib* vattrib );
void    flush_Vattrib_range( Vattrib* vattrib, GLintptr ofs, GLsizeiptr n );

int    upload_Vattrib( Vattrib* vattrib, GLenum usage, GLsizeiptr n, pointer data );
int    upload_Vattrib_range( Vattrib* vattrib, GLintptr ofs, GLsizeiptr n, pointer data );

#endif
