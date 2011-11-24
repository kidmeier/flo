#include <assert.h>
#include <stdarg.h>

#include <SDL.h>

#include "core.log.h"
#include "gl.display.h"

static SDL_Window* _display = NULL;

static int _width  = 0;
static int _height = 0;

static void maybe_initialize_video(void) {
	
	if( SDL_INIT_VIDEO != SDL_WasInit(SDL_INIT_VIDEO) )
		SDL_InitSubSystem( SDL_INIT_VIDEO );
	
}

Display* open_Display( const char* title, 
                       int width, int height, int flags,
                       int red, int blue, int green, int alpha,
                       int depth, int stencil,
                       int glMajor, int glMinor ) {

	if( NULL != _display ) {
		error0("Display can only be opened once!");
		return  NULL;
	}
	
	maybe_initialize_video();
	
#define param(user,dfault) ( ((user) < 0) ? (dfault) : (user) )    
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, param(red,8) );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, param(green,8) );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, param(blue,8) );
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, param(alpha,8) );

    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, param(depth,8) );
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, param(stencil,8) );

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, param(glMajor,2) );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, param(glMinor,1) );

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
#undef param

	_display = SDL_CreateWindow( title, 
	                            SDL_WINDOWPOS_UNDEFINED, 
	                            SDL_WINDOWPOS_UNDEFINED,
	                            width, 
	                            height, 
	                            flags | SDL_WINDOW_OPENGL );
	if( NULL == _display ) {
		
		error("Failed to open display: %s", SDL_GetError());
		return NULL;
		
	}

    _width  = width;
    _height = height;

	SDL_ShowWindow( _display );
	return _display;    

}

void     close_Display( Display* dpy ) {
	
	assert( _display == dpy );
	SDL_DestroyWindow( dpy );
	
}

void     flip_Display( Display* dpy ) {
	
	assert( _display == dpy );
	SDL_GL_SwapWindow( dpy );
	
}

float  aspect_Display( Display* dpy ) {

	assert( _display == dpy );
    return (float)_width / (float)_height;

}

int     width_Display( Display* dpy ) {

	assert( _display == dpy );
    return _width;

}

int    height_Display( Display* dpy ) {

	assert( _display == dpy );
    return _height;

}
