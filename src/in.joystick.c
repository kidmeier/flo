#include <assert.h>
#include <SDL/SDL.h>

#include "core.alloc.h"
#include "core.types.h"
#include "in.joystick.h"

static pointer     pool = NULL;

static int         n_joysticks = 0;
static joystick_p  joysticks = NULL;

static int init(void) {

	if( !pool ) {

		if( 0 == SDL_WasInit(SDL_INIT_JOYSTICK) ) {
			if( 0 < SDL_InitSubSystem( SDL_INIT_JOYSTICK ) )
				return -1;
		}

		pool = autofree_pool();

		// Enumerate joysticks and prepare them for events
		n_joysticks = SDL_NumJoysticks();
		joysticks = new_array( pool, struct joystick_s, n_joysticks );
		for( int i=0; i<n_joysticks; i++ ) {
			SDL_Joystick* joy      = SDL_JoystickOpen(i);
			joysticks[i].name      = SDL_JoystickName(i);
			joysticks[i].n_axes    = SDL_JoystickNumAxes( joy );
			joysticks[i].n_balls   = SDL_JoystickNumBalls( joy );
			joysticks[i].n_buttons = SDL_JoystickNumButtons( joy );
			joysticks[i].n_hats    = SDL_JoystickNumHats( joy );
		}

	}
	
}

// Public API 

int              joystick_count_IN(void) {

	if( init() < 0 )
		return 0;

	return n_joysticks;

}

const joystick_p joystick_info_IN( int n ) {

	if( init() < 0 ) 
		return NULL;

	assert( n >= 0 && n < n_joysticks );
	return &joysticks[n];

}
