#include <assert.h>
#include <SDL/SDL_events.h>

#include "core.string.h"
#include "ev.quit.h"

// WARNING: This is not re-entrant; should only be called from one thread.
int init_quit_EV( ev_t* dest, const union SDL_Event* ev ) {

	assert( SDL_QUIT == ev->type );
	// Quit has no data
	return 0;

}

int describe_quit_EV( ev_t* ev, int n, char* dest ) {

	const char* quit = "Quit";
	return maybe_strncpy( dest, n, quit );

}

int detail_quit_EV( ev_t* ev, int n, char* dest ) {

	return describe_quit_EV( ev, n, dest );

}
