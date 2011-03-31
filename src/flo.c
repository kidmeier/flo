#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

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
#include "core.log.h"
#include "core.system.h"

#include "gl.context.h"
#include "gl.display.h"

#include "ev.channel.h"
#include "ev.core.h"
#include "ev.axis.h"
#include "ev.cursor.h"
#include "ev.button.h"
#include "ev.keyboard.h"
#include "ev.focus.h"
#include "ev.window.h"
#include "job.channel.h"
#include "job.core.h"
#include "job.control.h"

#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.thread.h"

static bool quit_requested = false;

declare_job( void, window_ev_monitor, ev_channel_p evch );
define_job( void, window_ev_monitor, 

            Channel* source;
            Channel* passthru;
            ev_t     ev ) {

	begin_job;

	local(source)   = new_Channel( sizeof(ev_window_t), 16 );
	local(passthru) = push_EV_sink( arg(evch), local(source) );
	while( !quit_requested ) {

		readch_raw( local(source), sizeof(ev_window_t), &local(ev) );
		if( windowClosed == local(ev).window.what ) {

			quit_requested = true;

		} else {

			writech_raw( local(passthru), sizeof(ev_window_t), &local(ev) );

		}

	}
	pop_EV_sink( arg(evch) );
	
	end_job;

}

int main(int argc, char* argv[]) {

	int ret = init_FLO();

	Display* display = open_Display( "Flo",
	                                 512, 288, 0,

	                                 redBits, 8,
	                                 greenBits, 8,
	                                 blueBits, 8,
//	                                 alphaBits, 8,
	                                 
	                                 depthBits, 24,
	                                 
	                                 doubleBuffer, 1,
	                                 requireAccel, 1,

	                                 glMajor, 3,
	                                 glMinor, 2,

	                                 -1 );

	if( NULL == display ) {
		fatal0("Failed to open display");
		exit(1);
	}

	Glcontext gl = create_Glcontext( display );
	if( !gl ) {
		fatal0("Failed to create GL context");
		exit(1);
	}

	ret = maybe( ret, < 0, init_EV() );
	ret = maybe( ret, < 0, init_Jobs( cpu_count_SYS() ) );

	if( ret < 0 )
		fatal0("Failed to initialize runtime");

	struct ev_channel_s* axes = open_EV( axis_EV_adaptor );
	struct ev_channel_s* keyb = open_EV( kbd_EV_adaptor );
	struct ev_channel_s* buttons = open_EV( button_EV_adaptor );
	struct ev_channel_s* cursor = open_EV( cursor_EV_adaptor );
	struct ev_channel_s* focus = open_EV( focus_EV_adaptor );
	struct ev_channel_s* window = open_EV( window_EV_adaptor );

	// Start monitoring window events
	typeof_Job_params(window_ev_monitor) params = { window };
	submit_Job( 0, ioBound, NULL, (jobfunc_f)window_ev_monitor, &params );

	uint32 tick = 0;
	while( !(ret < 0) && !quit_requested ) {

		int events = pump_EV(tick);
		sleep_THREAD( usec_perSecond / 30 );

		tick++;

	}
	
	delete_Glcontext( gl );
	close_Display( display );

	return 0;

}

#endif
