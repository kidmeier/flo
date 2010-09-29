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
#include "core.system.h"
#include "display.core.h"

#include "ev.core.h"
#include "ev.axis.h"
#include "ev.cursor.h"
#include "ev.button.h"
#include "ev.keyboard.h"
#include "ev.focus.h"
#include "ev.window.h"
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
	ret = maybe( ret, < 0, init_Jobs( cpu_count_SYS() ) );

	struct ev_channel_s* keyb = open_EV( kbd_EV_adaptor );
	struct ev_channel_s* buttons = open_EV( button_EV_adaptor );
	struct ev_channel_s* cursor = open_EV( cursor_EV_adaptor );
	struct ev_channel_s* focus = open_EV( focus_EV_adaptor );
	struct ev_channel_s* window = open_EV( window_EV_adaptor );

	uint32 tick = 0;
	while( !(ret < 0) && !quit_requested_EV() ) {

		int events = pump_EV(tick);
		sleep_THREAD( usec_perSecond / 30 );

		tick++;

	}
	
	return 0;
}

#endif
