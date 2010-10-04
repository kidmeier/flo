#include <assert.h>

#include "core.alloc.h"
#include "gl.buf.h"

// Buffers ////////////////////////////////////////////////////////////////////

GLuint    new_Buf(void) {

	GLuint buf;
	glGenBuffers( 1, &buf );
	
	return buf;

}

void   delete_Buf( GLuint buf ) {

	glDeleteBuffers( 1, &buf );

}

pointer alloc_Buf( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size ) {
	
	glBindBuffer( target, buf );
	glBufferData( target, size, NULL, usage );
	
	return glMapBuffer( target, GL_READ_WRITE );
	
}

pointer   map_Buf( GLuint buf, GLenum target, GLenum access ) {

	glBindBuffer( target, buf);
	return glMapBuffer( target, access );
	
}

pointer   map_Buf_range( GLuint buf, GLenum target, GLbitfield access, GLintptr ofs, GLsizeiptr len ) {
	glBindBuffer( target, buf );
	return glMapBufferRange( target, ofs, len, access );

}

void    flush_Buf( GLuint buf, GLenum target ) {

	glUnmapBuffer( target );

}

void    flush_Buf_range( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr len ) {

	glFlushMappedBufferRange( target, ofs, len );

}

int    upload_Buf( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size, void* data ) {
	
	glBindBuffer( target, buf );
	glBufferData( target, size, data, usage );
	
	return 0;

}

int    upload_Buf_range( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr size, void* data ) {
	
	glBindBuffer( target, buf );
	glBufferSubData( target, ofs, size, data );
	
	return 0;

}
