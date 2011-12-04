#include "core.features.h"

#if defined( feature_DEBUG ) && defined( feature_POSIX )
#include <signal.h>
#endif

#include "core.log.h"
#include "gl.util.h"

GLenum gl_lastError;

static const char* describeError( GLenum err ) {

	switch( err ) {
		
	case GL_NO_ERROR:
		return "GL_NO_ERROR";

	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
		
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";

	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";

	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";

	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";

	default:
		fatal( "Unknown glGetError enum: %d", err );
		return NULL;
	}

}

GLenum _check_GL_error( const char* file, int lineno ) {
	
	gl_lastError = glGetError();
	if( gl_lastError != GL_NO_ERROR ) {
		
		printLog( logError, "GL ERROR: %s", file, lineno, describeError(gl_lastError) );
		
		// If we are attached to a debugged this will break into the debugger
#if defined( feature_DEBUG ) && defined( feature_POSIX )
		raise( SIGINT );
#endif
	}
	
	return gl_lastError;
	
}
