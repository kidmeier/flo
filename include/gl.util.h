#ifndef __gl_util_H__
#define __gl_util_H__

#include <GL/glew.h>
#include "core.features.h"

#if defined( feature_MINGW ) 
#undef GLAPIENTRY
#define GLAPIENTRY __stdcall
#elif !defined( GLAPIENTRY )
#define GLAPIENTRY
#endif

#if defined( feature_DEBUG ) || defined( feature_TRACE )

#define check_GL_error	  \
	_check_GL_error( __FILE__, __LINE__ )

#else

#define check_GL_error

#endif

extern GLenum gl_lastError;
GLenum _check_GL_error( const char* file, int lineno );

#endif
