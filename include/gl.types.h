#ifndef __gl_types_h__
#define __gl_types_h__

#include <GL/glew.h>
#include "core.types.h"

bool  decimal_GLtype( GLenum type );
bool integral_GLtype( GLenum type );
int    sizeof_GLtype( GLenum type );

#endif
