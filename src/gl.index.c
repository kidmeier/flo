#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "core.alloc.h"
#include "gl.buf.h"
#include "gl.index.h"

// Index buffers //////////////////////////////////////////////////////////////

struct Vindex {
	
	GLuint id;
	int*   buf;
	
};

Vindex* new_Vindex( void ) {
	
	GLuint id = new_Buf();
	if( 0 == id ) 
		return NULL;
	
	Vindex* vindex = new( NULL, Vindex );
	
	vindex->id = id;
	vindex->buf = NULL;
	
	return vindex;
}

void delete_Vindex( Vindex* vindex ) {
	
	delete_Buf( vindex->id );
	
	memset( vindex, 0, sizeof(Vindex) );
	delete( vindex );
	
}

int  upload_Vindex( Vindex* vindex, GLenum usage, uint n, int* data ) {
	
	return upload_Buf( vindex->id, GL_ELEMENT_ARRAY_BUFFER, usage, n * sizeof(uint), data );
	
}

int  upload_Vindex_range( Vindex* vindex, uint ofs, uint n, int* data ) {
	
	return upload_Buf_range( vindex->id, GL_ELEMENT_ARRAY_BUFFER, 
	                         ofs * sizeof(uint), 
	                         n * sizeof(uint),
	                         data );
}

int*  alloc_Vindex( Vindex* vindex, GLenum usage, uint n  ) {
	
	return (int*)alloc_Buf( vindex->id, GL_ELEMENT_ARRAY_BUFFER, usage, n * sizeof(uint) );
	
}

int*    map_Vindex( Vindex* vindex, GLenum access ) {
	
	assert( NULL == vindex->buf );
	
	vindex->buf = (int*)map_Buf( vindex->id, GL_ELEMENT_ARRAY_BUFFER, access );
	return vindex->buf;  
	
}

int*    map_Vindex_range( Vindex* vindex, uint ofs, uint n, GLenum rw, GLbitfield access ) {
	
	assert( NULL == vindex->buf );
	
	vindex->buf = (int*)map_Buf_range( vindex->id, GL_ELEMENT_ARRAY_BUFFER,
	                                   access,
	                                   ofs * sizeof(uint),
	                                   n * sizeof(uint) );
	
	return vindex->buf;

}

void  flush_Vindex( Vindex* vindex ) {
	
	assert( NULL != vindex->buf );
	
	flush_Buf( vindex->id, GL_ELEMENT_ARRAY_BUFFER );
	vindex->buf = NULL;

}

void  flush_Vindex_range( Vindex* vindex, uint ofs, uint n ) {
	
	assert( NULL != vindex->buf );
	
	flush_Buf_range( vindex->id, GL_ELEMENT_ARRAY_BUFFER, 
	                 ofs * sizeof(uint),
	                 n * sizeof(uint) );

}
