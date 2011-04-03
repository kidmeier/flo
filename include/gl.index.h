#ifndef __gl_index_h__
#define __gl_index_h__

#include "gl.buf.h"

// Index buffers //////////////////////////////////////////////////////////////

typedef struct Vindex  Vindex;
struct Vindex {
	
	GLuint  id;

	GLenum  type;
	pointer buf;

};

Vindex*   new_Vindex( GLenum type );
void   delete_Vindex( Vindex* vindex );

int    upload_Vindex( Vindex* vindex, GLenum usage, GLsizeiptr n, pointer );
int    upload_Vindex_range( Vindex* vindex, 
                          GLintptr ofs, 
                          GLsizeiptr n, 
                          pointer data );

pointer alloc_Vindex( Vindex* vindex, GLenum usage, GLsizeiptr n );
pointer   map_Vindex( Vindex* vindex, GLenum access );
pointer   map_Vindex_range( Vindex* vindex, 
                            GLintptr ofs, 
                            GLsizeiptr n, 
                            GLenum rw, 
                            GLbitfield access );
void    flush_Vindex( Vindex* vindex );
void    flush_Vindex_range( Vindex* vindex, GLintptr ofs, GLsizeiptr N );

#endif
