#ifndef __gl_util_H__
#define __gl_util_H__

#include <GL/glew.h>

#ifdef _DEBUG

#define GL_check_error				\
  _GL_check_error( __FILE__, __LINE__ )

#else

#define GL_check_error

#endif

extern GLenum gl_error;
GLenum _GL_check_error( const char* file, int lineno );

#endif
