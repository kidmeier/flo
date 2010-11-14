#include <assert.h>
#include <SDL_events.h>

#include "core.log.h"
#include "core.string.h"
#include "ev.window.h"

//#define sdl_ev_mask SDL_VIDEORESIZEMASK | SDL_VIDEOEXPOSEMASK

static uint8 init_window_EV( enable_ev_f enable,
                             disable_ev_f disable,
                             va_list args ) {

	enable( SDL_WINDOWEVENT );
	return 0;

//	return sdl_ev_mask;

}

// WARNING: This is not re-entrant; should only be called from one thread.
static int translate_window_EV( ev_t* dest, const union SDL_Event* ev ) {

//	assert( 0 != (SDL_EVENTMASK(ev->type) & sdl_ev_mask) );

	static uint16 x = 0;
	static uint16 y = 0;
	
	static uint16 width = 0;
	static uint16 height = 0;

	assert( SDL_WINDOWEVENT == ev->type );

	switch( ev->window.event ) {

	case SDL_WINDOWEVENT_SHOWN:
		dest->window.what = windowShown;
		break;

	case SDL_WINDOWEVENT_HIDDEN:
		dest->window.what = windowHidden;
		break;

	case SDL_WINDOWEVENT_EXPOSED:
		dest->window.what = windowExposed;
		break;

	case SDL_WINDOWEVENT_MOVED:
		dest->window.what = windowMoved;
		x = (uint16)ev->window.data1;
		y = (uint16)ev->window.data2;
		break;

	case SDL_WINDOWEVENT_RESIZED:
		dest->window.what = windowResized;
		width = (uint16)ev->window.data1;
		height = (uint16)ev->window.data2;
		break;

	case SDL_WINDOWEVENT_MINIMIZED:
		dest->window.what = windowMinimized;
		break;

	case SDL_WINDOWEVENT_MAXIMIZED:
		dest->window.what = windowMaximized;
		break;

	case SDL_WINDOWEVENT_RESTORED:
		dest->window.what = windowRestored;
		break;

	case SDL_WINDOWEVENT_CLOSE:
		dest->window.what = windowClosed;
		break;

	default:
		fatal( "Bad window event: 0x%x", ev->window.event);
		break;
	}

	dest->window.position.x = x;
	dest->window.position.y = y;

	dest->window.size.width = width;
	dest->window.size.height = height;

	return 0;

}

static int describe_window_EV( ev_t* ev, int n, char* dest ) {
  
	char buf[256] = { '\0' };

	switch( ev->window.what )
	{
	case windowShown:
		strcpy( buf, "Shown" );
		break;
		
	case windowHidden:
		strcpy( buf, "Hidden" );
		break;
		
	case windowExposed:
		strcpy( buf, "Exposed" );
		break;
		
	case windowMoved:
		strcpy( buf, "Moved" );
		break;
		
	case windowResized:
		sprintf( buf, "Resize (%ux%u)", 
		         ev->window.size.width, 
		         ev->window.size.height);
		break;
		
	case windowMinimized:
		strcpy( buf, "Minimized" );
		break;
		
	case windowMaximized:
		strcpy( buf, "Maximized" );
		break;
		
	case windowRestored:
		strcpy( buf, "Restored" );
		break;
		
	case windowClosed:
		strcpy( buf, "Closed" );
		break;

	default:
		fatal( "Bad window event: 0x%x", ev->window.what);
	}
	
	return maybe_strncpy( dest, n, buf );
}

static int detail_window_EV( ev_t* ev, int n, char* dest ) {

	char buf[512];
	char desc[512];

	describe_window_EV( ev, sizeof(desc), desc );
	
	sprintf( buf, "Window (%u,%u @ %ux%u) %s", 
	         ev->window.position.x, ev->window.position.y,
	         ev->window.size.width, ev->window.size.height, 
	         desc );
	
	return maybe_strncpy( dest, n, buf );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evWindow,
	.ev_size      = sizeof(ev_window_t),
//	.ev_mask      = sdl_ev_mask,
	
	.init_ev      = init_window_EV,
	.translate_ev = translate_window_EV,
	.describe_ev  = describe_window_EV,
	.detail_ev    = detail_window_EV

};
ev_adaptor_p       window_EV_adaptor = &adaptor;
