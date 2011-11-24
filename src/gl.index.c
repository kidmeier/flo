#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "gl.buf.h"
#include "gl.index.h"
#include "gl.types.h"

inline static unsigned int indiceMax( GLenum type ) {

	return ~( 1UL << (8UL * sizeof_GLtype(type)) );
}

// Index buffers //////////////////////////////////////////////////////////////

Vindex*   new_Vindex( GLenum type ) {
	
	GLuint id = new_Buf();
	if( 0 == id ) 
		return NULL;
	
	Vindex* vindex = malloc( sizeof(Vindex) );
	
	vindex->id = id;

	vindex->type = type;
	vindex->buf  = NULL;
	
	return vindex;
}

void   delete_Vindex( Vindex* vindex ) {
	
	delete_Buf( vindex->id );
	
	memset( vindex, 0, sizeof(Vindex) );
	free( vindex );
	
}

int    upload_Vindex( Vindex* vindex, 
                      GLenum usage, 
                      GLsizeiptr n, 
                      pointer data ) {
	
	assert( integral_GLtype( vindex->type ) );
	assert( n <= indiceMax(vindex->type) );

	return upload_Buf( vindex->id, 
	                   GL_ELEMENT_ARRAY_BUFFER, 
	                   usage, 
	                   n * sizeof_GLtype(vindex->type), 
	                   data );
	
}

int    upload_Vindex_range( Vindex* vindex, 
                            GLintptr ofs, 
                            GLsizeiptr n, 
                            pointer data ) {

	assert( ofs <= indiceMax( vindex->type ) );
	assert( n   <= indiceMax( vindex->type ) );

	return upload_Buf_range( vindex->id, GL_ELEMENT_ARRAY_BUFFER, 
	                         ofs * sizeof_GLtype( vindex->type ), 
	                         n   * sizeof_GLtype( vindex->type ),
	                         data );
}

pointer alloc_Vindex( Vindex* vindex, GLenum usage, GLsizeiptr n  ) {
	

	assert( n   <= indiceMax( vindex->type ) );
	vindex->buf = alloc_Buf( vindex->id, 
	                         GL_ELEMENT_ARRAY_BUFFER, 
	                         usage, 
	                         n * sizeof_GLtype( vindex->type ) );
	return vindex->buf;

}

pointer   map_Vindex( Vindex* vindex, GLenum access ) {
	
	assert( NULL == vindex->buf );
	
	vindex->buf = map_Buf( vindex->id, GL_ELEMENT_ARRAY_BUFFER, access );
	return vindex->buf;  
	
}

pointer   map_Vindex_range( Vindex* vindex, 
                            GLintptr ofs, 
                            GLsizeiptr n, 
                            GLenum rw, 
                            GLbitfield access ) {
	
	assert( NULL == vindex->buf );
	assert( ofs <= indiceMax( vindex->type ) );
	assert( n   <= indiceMax( vindex->type ) );
	
	vindex->buf = map_Buf_range( vindex->id, GL_ELEMENT_ARRAY_BUFFER,
	                             access,
	                             ofs * sizeof_GLtype( vindex->type ),
	                             n   * sizeof_GLtype( vindex->type ) );
	
	return vindex->buf;

}

void    flush_Vindex( Vindex* vindex ) {
	
	assert( NULL != vindex->buf );
	
	flush_Buf( vindex->id, GL_ELEMENT_ARRAY_BUFFER );
	vindex->buf = NULL;
	
}

void    flush_Vindex_range( Vindex* vindex, GLintptr ofs, GLsizeiptr n ) {
	
	assert( NULL != vindex->buf );
	assert( ofs <= indiceMax( vindex->type ) );
	assert( n   <= indiceMax( vindex->type ) );
	
	flush_Buf_range( vindex->id, GL_ELEMENT_ARRAY_BUFFER,
	                 ofs * sizeof_GLtype( vindex->type ),
	                 n   * sizeof_GLtype( vindex->type ) );

}
