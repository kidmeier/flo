#ifndef __gl_debug_h__
#define __gl_debug_h__

#include <GL/glew.h>

// This generates a GL_INVALID_OPERATION error; tools such as 'BuGLe'
// can be configured to drop into a debugger when a GL error occurs,
// so this behaves like a progammatic breakpoint
#define glBreakpoint	  \
	glBegin( GL_TRIANGLES ); glEnableVertexAttribArray( 0 ); glEnd()


#endif
