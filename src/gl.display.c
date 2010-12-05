#include <assert.h>
#include <stdarg.h>

#include <SDL.h>

#include "core.log.h"
#include "gl.display.h"

static SDL_Window* display = NULL;

static void maybe_initialize_video(void) {
	
	if( SDL_INIT_VIDEO != SDL_WasInit(SDL_INIT_VIDEO) )
		SDL_InitSubSystem( SDL_INIT_VIDEO );
	
}

Display* open_Display( const char* title, 
                       int width, 
                       int height, 
                       int flags, ... ) {
	
	if( NULL != display ) {
		error0("Display can only be opened once!");
		return  NULL;
	}
	
	va_list attribs;
	
	maybe_initialize_video();
	
	va_start(attribs, flags);
	while( true ) {
		
		int attrib = va_arg(attribs, display_attrib_e);
		if( attrib < 0 )
			break;
		int value = va_arg(attribs, int);
		
		SDL_GL_SetAttribute( attrib, value );
		
	}
	va_end(attribs);
	
	display = SDL_CreateWindow( title, 
	                            SDL_WINDOWPOS_UNDEFINED, 
	                            SDL_WINDOWPOS_UNDEFINED,
	                            width, 
	                            height, 
	                            flags | SDL_WINDOW_OPENGL );
	if( NULL == display ) {
		
		error("Failed to open display: %s", SDL_GetError());
		return NULL;
		
	}
	
	SDL_ShowWindow( display );
	return display;
}

void     close_Display( Display* dpy ) {
	
	assert( display == dpy );
	SDL_DestroyWindow( dpy );
	
}

void     flip_Display( Display* dpy ) {
	
	assert( display == dpy );
	SDL_GL_SwapWindow( dpy );
	
}
