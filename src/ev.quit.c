#include <assert.h>
#include <SDL_events.h>

#include "core.string.h"
#include "ev.quit.h"

//#define sdl_ev_mask SDL_QUITMASK

static uint8 init_quit_EV( enable_ev_f enable,
                           disable_ev_f disable,
                           va_list args ) {
	
	enable( SDL_QUIT );
	return 0;

//	return sdl_ev_mask;

}

static int translate_quit_EV( ev_t* dest, const union SDL_Event* ev ) {

//	assert( 0 != (SDL_EVENTMASK(ev->type) & sdl_ev_mask) );

	// Quit has no data
	return 0;

}

static int describe_quit_EV( ev_t* ev, int n, char* dest ) {

	const char* quit = "Quit";
	return maybe_strncpy( dest, n, quit );

}

static int detail_quit_EV( ev_t* ev, int n, char* dest ) {

	return describe_quit_EV( ev, n, dest );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evQuit,
	.ev_size      = sizeof(ev_quit_t),
//	.ev_mask      = sdl_ev_mask,

	.init_ev      = init_quit_EV,
	.translate_ev = translate_quit_EV,
	.describe_ev  = describe_quit_EV,
	.detail_ev    = detail_quit_EV

};
ev_adaptor_p       quit_EV_adaptor = &adaptor;
