#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "core.alloc.h"
#include "gl.buf.h"
#include "gl.index.h"

// Index buffers //////////////////////////////////////////////////////////////

struct vindex_s {
	
	GLuint id;
	int*   buf;
	
};

vindex_p  new_VINDEX( void ) {
	
	GLuint id = new_BUF();
	if( 0 == id ) 
		return NULL;
	
	vindex_p vindex = new( NULL, vindex_t );
	
	vindex->id = id;
	vindex->buf = NULL;
	
	return vindex;
}

void    delete_VINDEX( vindex_p vindex ) {
	
	delete_BUF( vindex->id );
	
	memset( vindex, 0, sizeof(vindex_t) );
	delete( vindex );
	
}

int     upload_VINDEX( vindex_p vindex, GLenum usage, uint n, int* data ) {
	
	return upload_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, usage, n * sizeof(uint), data );
	
}

int     upload_range_VINDEX( vindex_p vindex, uint ofs, uint n, int* data ) {
	
	return upload_range_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, 
	                         ofs * sizeof(uint), 
	                         n * sizeof(uint),
	                         data );
}

int*    alloc_VINDEX( vindex_p vindex, GLenum usage, uint n  ) {
	
	return (int*)alloc_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, usage, n * sizeof(uint) );
	
}

int*    map_VINDEX( vindex_p vindex, GLenum access ) {
	
	assert( NULL == vindex->buf );
	
	vindex->buf = (int*)map_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, access );
	return vindex->buf;  
	
}

int*    map_range_VINDEX( vindex_p vindex, uint ofs, uint n, GLenum rw, GLbitfield access ) {
	
	assert( NULL == vindex->buf );
	
	vindex->buf = (int*)map_range_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER,
	                                   access,
	                                   ofs * sizeof(uint),
	                                   n * sizeof(uint) );
	
	return vindex->buf;

}

void    flush_VINDEX( vindex_p vindex ) {
	
	assert( NULL != vindex->buf );
	
	flush_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER );
	vindex->buf = NULL;

}

void    flush_range_VINDEX( vindex_p vindex, uint ofs, uint n ) {
	
	assert( NULL != vindex->buf );
	
	flush_range_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, 
	                 ofs * sizeof(uint),
	                 n * sizeof(uint) );

}
