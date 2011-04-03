#include <assert.h>

#include "core.log.h"
#include "gl.types.h"

bool  decimal_GLtype( GLenum type ) {

	return !( integral_GLtype(type) );

}

bool integral_GLtype( GLenum type ) {

	switch( type ) {

	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_INT:
	case GL_UNSIGNED_INT:
		return true;

	default:
		return false;

	}

}

int    sizeof_GLtype( GLenum type ) {
	
	switch( type ) {
		
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
		return 1;

	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
		return 2;

	case GL_INT:
	case GL_UNSIGNED_INT:
	case GL_FLOAT:
		return 4;

	case GL_DOUBLE:
		return 8;

	default:
		assert( 0 && "`type' must be one of: GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT, GL_FLOAT, GL_DOUBLE" );
		break;
	}

	return -1;

}
