#ifndef __gl_geom_h__
#define __gl_geom_h__

#include <GL/glew.h>
#include "core.types.h"

enum gl_buf_access_e {

	readOnly  = GL_READ_ONLY,
	writeOnly = GL_WRITE_ONLY,
	readWrite = GL_READ_WRITE

};

enum gl_buf_usage_e {
	
	streamDraw  = GL_STREAM_DRAW,
	streamRead  = GL_STREAM_READ,
	streamCopy  = GL_STREAM_COPY,

	staticDraw  = GL_STATIC_DRAW,
	staticRead  = GL_STATIC_READ,
	staticCopy  = GL_STATIC_COPY,

	dynamicDraw  = GL_DYNAMIC_DRAW,
	dynamicRead  = GL_DYNAMIC_READ,
	dynamicCopy  = GL_DYNAMIC_COPY,

};

// Raw buffers ////////////////////////////////////////////////////////////////
GLuint    new_Buf(void);
void   delete_Buf( GLuint buf );

pointer alloc_Buf( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size );
pointer   map_Buf( GLuint buf, GLenum target, GLenum access );
pointer   map_Buf_range( GLuint buf, GLenum target, GLbitfield access, GLintptr ofs, GLsizeiptr len );
void    flush_Buf( GLuint buf, GLenum target );
void    flush_Buf_range( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr len );

int    upload_Buf( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size, pointer data );
int    upload_Buf_range( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr size, pointer data );

#endif
