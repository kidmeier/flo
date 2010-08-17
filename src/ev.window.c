#include <assert.h>
#include <SDL/SDL_events.h>

#include "core.string.h"
#include "ev.window.h"

#define sdl_ev_mask SDL_VIDEORESIZEMASK | SDL_VIDEOEXPOSEMASK

static uint32 init_window_EV( va_list args ) {

	return sdl_ev_mask;

}

// WARNING: This is not re-entrant; should only be called from one thread.
static int translate_window_EV( ev_t* dest, const union SDL_Event* ev ) {

	assert( 0 != (SDL_EVENTMASK(ev->type) & sdl_ev_mask) );

	static uint16 width = 0;
	static uint16 height = 0;

	switch( ev->type ) {

	case SDL_VIDEORESIZE:
		dest->window.width = width = (uint16)ev->resize.w;
		dest->window.height = height = (uint16)ev->resize.h;
		dest->window.exposed = false;
		break;

	case SDL_VIDEOEXPOSE:
		dest->window.width = width;
		dest->window.height = height;
		dest->window.exposed = true;
		break;

	}

	return 0;

}

static int describe_window_EV( ev_t* ev, int n, char* dest ) {

	char buf[256] = { '\0' };

	if( ev->window.exposed )
		strcpy( buf, "Exposed" );
	else 
		sprintf( buf, "Resize (%ux%u)", ev->window.width, ev->window.height);

	return maybe_strncpy( dest, n, buf );

}

static int detail_window_EV( ev_t* ev, int n, char* dest ) {

	char buf[512];

	sprintf( buf, "Window (%ux%u) %s", 
	         ev->window.width, ev->window.height, 
	         (ev->window.exposed ? "Exposed" : "Resized"));

	return maybe_strncpy( dest, n, buf );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evWindow,
	.ev_size      = sizeof(ev_window_t),
	.ev_mask      = sdl_ev_mask,

	.init_ev      = init_window_EV,
	.translate_ev = translate_window_EV,
	.describe_ev  = describe_window_EV,
	.detail_ev    = detail_window_EV

};
ev_adaptor_p       window_EV_adaptor = &adaptor;
