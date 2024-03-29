#include <assert.h>
#include <SDL_events.h>

#include "core.string.h"
#include "ev.quit.h"

static uint8 init_quit_Ev( enable_ev_f enable,
                           disable_ev_f disable,
                           va_list args ) {
	
	enable( SDL_QUIT );
	return 0;

}

static int translate_quit_Ev( ev_t* dest, const union SDL_Event* ev ) {

	assert( SDL_QUIT == ev->type );

	// Quit has no data
	return 0;

}

static int describe_quit_Ev( const ev_t* ev, int n, char* dest ) {

	const char* quit = "Quit";
	return maybe_strncpy( dest, n, quit );

}

static int detail_quit_Ev( const ev_t* ev, int n, char* dest ) {

	return describe_quit_Ev( ev, n, dest );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evQuit,
	.ev_size      = sizeof(ev_quit_t),

	.init_ev      = init_quit_Ev,
	.translate_ev = translate_quit_Ev,
	.describe_ev  = describe_quit_Ev,
	.detail_ev    = detail_quit_Ev

};
ev_adaptor_p       quit_Ev_adaptor = &adaptor;
