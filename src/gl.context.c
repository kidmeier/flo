#include <GL/glew.h>
#include <SDL_video.h>

#include "core.log.h"
#include "gl.context.h"
#include "gl.display.h"

Glcontext create_Glcontext( Display* dpy ) {

	Glcontext ctx = SDL_GL_CreateContext( dpy );
	if( !ctx )
		return ctx;

	debug("OpenGL %s", glGetString( GL_VERSION ) );
	debug("  vendor:   %s", glGetString( GL_VENDOR ) );
	debug("  renderer: %s", glGetString( GL_RENDERER ) );
	debug("  GLSL:     %s", glGetString( GL_SHADING_LANGUAGE_VERSION ) );

	GLenum err = glewInit();
	if( GLEW_OK != err ) {

		fatal("glewInit failed: %s", glewGetErrorString(err));
		return NULL;

	} else {

		debug("initialized GLEW v%s", glewGetString(GLEW_VERSION));
		if( GLEW_VERSION_1_1 ) debug0("have OpenGL v1.1");
		if( GLEW_VERSION_1_2 ) debug0("have OpenGL v1.2");
		if( GLEW_VERSION_1_3 ) debug0("have OpenGL v1.3");
		if( GLEW_VERSION_1_4 ) debug0("have OpenGL v1.4");
		if( GLEW_VERSION_1_5 ) debug0("have OpenGL v1.5");
		if( GLEW_VERSION_2_0 ) debug0("have OpenGL v2.0");
		if( GLEW_VERSION_2_1 ) debug0("have OpenGL v2.1");
		if( GLEW_VERSION_3_0 ) debug0("have OpenGL v3.0");
		if( GLEW_VERSION_3_1 ) debug0("have OpenGL v3.1");
		if( GLEW_VERSION_3_2 ) debug0("have OpenGL v3.2");

	}

	return ctx;

}

void      delete_Glcontext( Glcontext gl ) {

	SDL_GL_DeleteContext( gl );

}

int         bind_Glcontext( Display* dpy, Glcontext gl ) {

	int ret = SDL_GL_MakeCurrent( dpy, gl );
	if( ret < 0 )
		error("bind_Glcontext failed: %s", SDL_GetError());

	return ret;

}
