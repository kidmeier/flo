#include "core.log.h"
#include "gl.context.headless.h"

#if defined( feature_X11 ) && defined( feature_GLX )

// Ref: http://sidvind.com/wiki/Opengl/windowless
Glcontext_headless create_Glcontext_headless( void ) {

	/* open display */
	Display *dpy = XOpenDisplay(NULL);
	if ( !dpy ) {
		error0( "Cannot connect to X server");
		return NULL;
	}
	
	/* get root window */
	Window root = DefaultRootWindow(dpy);
	
	/* get visual matching attr */
	GLint attr[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	XVisualInfo *vi = glXChooseVisual(dpy, 0, attr);
	if( !vi ) {
		error0("No appropriate visual found");
		return NULL;
	}
	
	/* create a context using the root window */
	GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);	

	if ( !glc ) {
		error0("Failed to create context");
		return NULL;
	}
	glXMakeCurrent(dpy, root, glc);

	return glc;

}

void              destroy_Glcontext_headless( Glcontext_headless glc ) {

	/* open display */
	Display *dpy = XOpenDisplay(NULL);
	if ( !dpy ) {
		error0( "Cannot connect to X server");
		return;
	}
	glXDestroyContext( dpy, glc );

}

#else

Glcontext_headless create_Glcontext_headless( void ) {

	fatal0( "create_Glcontext_headless() is not supported on this platform" );
	return NULL:

}

void              destroy_Glcontext_headless( Glcontext_headless glc ) {

	fatal0( "destroy_Glcontext_headless() is not supported on this platform" );

}

#endif

#ifdef __gl_context_headless_TEST__

int main( int argc, char *argv[] ) {

	Glcontext_headless glc = create_Glcontext_headless();
	if( !glc )
		return 1;

	debug( "Created headless context: 0x%p", glc );

	destroy_Glcontext_headless( glc );
	return 0;

}

#endif
