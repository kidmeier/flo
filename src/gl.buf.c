#include <assert.h>

#include "core.alloc.h"
#include "gl.buf.h"

// Buffers ////////////////////////////////////////////////////////////////////

GLuint new_BUF(void) {

	GLuint buf;
	glGenBuffers( 1, &buf );
	
	return buf;

}

void   delete_BUF( GLuint buf ) {

	glDeleteBuffers( 1, &buf );

}

void*  alloc_BUF( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size ) {
	
	glBindBuffer( target, buf );
	glBufferData( target, size, NULL, usage );
	
	return glMapBuffer( target, GL_READ_WRITE );
	
}

void*  map_BUF( GLuint buf, GLenum target, GLenum access ) {

	glBindBuffer( target, buf);
	return glMapBuffer( target, access );
	
}

void*  map_range_BUF( GLuint buf, GLenum target, GLbitfield access, GLintptr ofs, GLsizeiptr len ) {
	glBindBuffer( target, buf );
	return glMapBufferRange( target, ofs, len, access );

}

void   flush_BUF( GLuint buf, GLenum target ) {

	glUnmapBuffer( target );

}

void   flush_range_BUF( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr len ) {

	glFlushMappedBufferRange( target, ofs, len );

}

int     upload_BUF( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size, void* data ) {
	
	glBindBuffer( target, buf );
	glBufferData( target, size, data, usage );
	
	return 0;

}

int     upload_range_BUF( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr size, void* data ) {
	
	glBindBuffer( target, buf );
	glBufferSubData( target, ofs, size, data );
	
	return 0;

}
