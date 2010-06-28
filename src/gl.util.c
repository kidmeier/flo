#include <stdio.h>
#include "gl.util.h"

GLenum gl_error;

GLenum _GL_check_error( const char* file, int lineno ) {

  gl_error = glGetError();
  if( gl_error != GL_NO_ERROR ) {
    
    const GLubyte* err = gluErrorString(gl_error);
    fprintf(stderr, "%s:%d\t%s\n", file, lineno, err);

  }

  return gl_error;
}
