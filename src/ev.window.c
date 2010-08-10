#include <assert.h>
#include <SDL/SDL_events.h>

#include "core.string.h"
#include "ev.window.h"

// WARNING: This is not re-entrant; should only be called from one thread.
int init_window_EV( ev_t* dest, const union SDL_Event* ev ) {

	static uint16 width = 0;
	static uint16 height = 0;

	assert( SDL_VIDEORESIZE == ev->type 
		|| SDL_VIDEOEXPOSE == ev->type );

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

int describe_window_EV( ev_t* ev, int n, char* dest ) {

	char buf[256] = { '\0' };

	if( ev->window.exposed )
		strcpy( buf, "Exposed" );
	else 
		sprintf( buf, "Resize (%ux%u)", ev->window.width, ev->window.height);

	return maybe_strncpy( dest, n, buf );

}

int detail_window_EV( ev_t* ev, int n, char* dest ) {

	char buf[512];

	sprintf( buf, "Window (%ux%u) %s", 
	         ev->window.width, ev->window.height, 
	         (ev->window.exposed ? "Exposed" : "Resized"));

	return maybe_strncpy( dest, n, buf );

}
