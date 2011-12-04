#include <assert.h>

#include "core.alloc.h"
#include "gl.buf.h"
#include "gl.util.h"

// Buffers ////////////////////////////////////////////////////////////////////

GLuint    new_Buf(void) {

	GLuint buf;
	glGenBuffers( 1, &buf ); 
	check_GL_error;
	
	return buf;

}

void   delete_Buf( GLuint buf ) {

	glDeleteBuffers( 1, &buf );	check_GL_error;

}

pointer alloc_Buf( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size ) {
	
	glBindBuffer( target, buf ); check_GL_error;
	glBufferData( target, size, NULL, usage ); check_GL_error;
	
	return glMapBuffer( target, GL_READ_WRITE ); check_GL_error;
	
}

pointer   map_Buf( GLuint buf, GLenum target, GLenum access ) {

	glBindBuffer( target, buf); check_GL_error;
	return glMapBuffer( target, access ); check_GL_error;
	
}

pointer   map_Buf_range( GLuint buf, GLenum target, GLbitfield access, GLintptr ofs, GLsizeiptr len ) {
	glBindBuffer( target, buf ); check_GL_error;
	return glMapBufferRange( target, ofs, len, access ); check_GL_error;

}

void    flush_Buf( GLuint buf, GLenum target ) {

	glBindBuffer( target, buf ); check_GL_error;
	glUnmapBuffer( target ); check_GL_error;

}

void    flush_Buf_range( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr len ) {

	glFlushMappedBufferRange( target, ofs, len ); check_GL_error;

}

int    upload_Buf( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size, void* data ) {
	
	glBindBuffer( target, buf ); check_GL_error;
	glBufferData( target, size, data, usage ); check_GL_error;
	
	return 0;

}

int    upload_Buf_range( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr size, void* data ) {
	
	glBindBuffer( target, buf ); check_GL_error;
	glBufferSubData( target, ofs, size, data ); check_GL_error;
	
	return 0;

}
