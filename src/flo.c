#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>

int init_FLO( void ) {

	// Initialize SDL
	if( SDL_Init( SDL_INIT_NOPARACHUTE ) < 0 ) {
		
		fprintf(stderr, "Error: SDL_Init(): %s\n", SDL_GetError());
		exit(1);
		
	}

	return 0;

}

#ifndef __TEST__

#include "control.maybe.h"
#include "display.core.h"
#include "ev.core.h"
#include "job.core.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.thread.h"

int main(int argc, char* argv[]) {

	int ret = init_FLO();

	if( set_DISPLAY( 512, 288, 0, 0,
	                 redBits, 8,
	                 greenBits, 8,
	                 blueBits, 8,
	                 alphaBits, 8,
	                 
	                 depthBits, 24,
	                 
	                 doubleBuffer, 1,
	                 vsync, 1,
	                 requireAccel, 1,
	                 -1) < 0 ) {
		fprintf(stderr, "Error: Failed to open display\n");
		exit(1);
	}
	
	ret = maybe( ret, < 0, init_EV() );
	ret = maybe( ret, < 0, init_JOBS() );

	mutex_t mutex;
	condition_t signal;

	ret = maybe( ret, < 0, init_MUTEX(&mutex) );
	ret = maybe( ret, < 0, init_CONDITION(&signal) );

	struct ev_channel_s* keyb = open_EV( evKeyboard );
	struct ev_channel_s* mouse = open_EV( evMouse );
	while( !(ret < 0) ) {

		int events = pump_EV();
		sleep_THREAD( usec_perSecond / 30 );

	}
	join_deadline_JOB( 0, &mutex, &signal );
	
	destroy_MUTEX(&mutex);
	destroy_CONDITION(&signal);

	return 0;
}

#endif
