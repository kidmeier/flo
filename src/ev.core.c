#include <assert.h>
#include <SDL/SDL_events.h>

#include "core.types.h"

#include "ev.core.h"
#include "ev.channel.h"

#include "job.control.h"
#include "core.alloc.h"

// Forward decls
declare_job( void, ev_echo, job_channel_p source; int ev_size );

// SDL data wrangling

static uint32 SDL_ev_filter_mask;

static int SDL_ev_filter( const SDL_Event* ev ) {

	// Simple; yes if type is in the mask; no otherwise
	return ( 0 != (SDL_EVENTMASK(ev->type) & SDL_ev_filter_mask) );

}

static enum ev_type_e SDL_ev_type( const SDL_Event* ev ) {

	switch( ev->type ) {

	case SDL_ACTIVEEVENT:
		return evFocus;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		return evKeyboard;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		return evButton;

	case SDL_JOYAXISMOTION:

	case SDL_MOUSEMOTION:
	case SDL_JOYBALLMOTION:
		return evCursor;

	case SDL_JOYHATMOTION:
		return evDpad;

	case SDL_VIDEORESIZE:
	case SDL_VIDEOEXPOSE:
		return evWindow;
		
	case SDL_QUIT:
		return evQuit;
		
	case SDL_SYSWMEVENT:
		return evPlatform;

		// TODO?
	case SDL_USEREVENT:
	default:
		return evUnknown;
	}

}

static void init_SDL_ev(void) {

	SDL_ev_filter_mask = SDL_EVENTMASK(SDL_QUIT); // We always listen for quit
	SDL_SetEventFilter( SDL_ev_filter );

}

// Internal data //////////////////////////////////////////////////////////////

struct ev_device_s {

	job_channel_p        sink;
	jobid                job;
	ev_echo_job_params_t params;

};

static ev_adaptor_p ev_adaptors[evTypeCount];
static struct ev_device_s devices[evTypeCount];
static ev_channel_p       ev_channels[ evTypeCount ];

static msec_t             base_ev_time = 0;
static bool               quit_requested = false;

// Root event handler /////////////////////////////////////////////////////////

// Echo job; This is the base-level `sink` installed at the bottom of each 
// ev_channel_p. It simply echoes the `detail` string of the event to stdout.
define_job( void, ev_echo, 

            ev_t                 ev;
            char                 ev_desc[4092];
            struct ev_adaptor_s* adaptor

	) {

	begin_job;
	
	while(1) {

		readch_raw( arg(source), arg(ev_size), &local(ev) );

		local(adaptor) = ev_adaptors[local(ev).info.type];
		if( local(adaptor)->detail_ev( &local(ev), sizeof(local(ev_desc)), local(ev_desc) ) > 0 ) {
			fprintf(stdout, "[EV] % 8.4fs %s\n", 
			        (double)local(ev).info.time / usec_perSecond, 
			        local(ev_desc));
		}

	}

	end_job;
}

// Public API /////////////////////////////////////////////////////////////////

int init_EV( void ) {

	memset( &ev_channels, 0, sizeof(ev_channels) );
	memset( &devices, 0, sizeof(devices) );
	memset( &ev_adaptors, 0, sizeof(ev_adaptors) );
	init_SDL_ev();

	quit_requested = false;
	base_ev_time = microseconds();
	return 0;

}

int pump_EV( uint32 tick ) {

	const static int numEvents = 16;
	SDL_Event events[ numEvents ];

	SDL_PumpEvents();

	// The event pump works as follows:
	// 1. Query SDL for events
	// 2. Convert to native event representation
	// 3. Lookup event channel
	// 4. Write it to the event channel's sink
	// 5. Repeat until no more events in SDL queue
	int total = 0;
	while( true ) {

		int count = SDL_PeepEvents(&events[0], numEvents, SDL_GETEVENT, SDL_ev_filter_mask);
		for( int i=0; i<count; i++ ) {

			const SDL_Event* sdl_ev = &events[i];
			enum ev_type_e     type = SDL_ev_type(sdl_ev);
			ev_adaptor_p    adaptor = ev_adaptors[type];
			ev_channel_p     evchan = ev_channels[type];
			ev_t ev;

			// Check for QUIT and flag it
			if( SDL_QUIT == sdl_ev->type ) {

				quit_requested = true;
				// Application is not listening for SDL_QUIT, skip
				if( NULL == evchan )
					continue;

			} else {
				assert( NULL != evchan );
				assert( NULL != adaptor );
			}
			
			// Stamp the event
			ev.info.time = microseconds() - base_ev_time;
			ev.info.tick = tick;
			ev.info.type = type;

			// Adapt to our ev representation
			job_channel_p chan = peek_EV_sink( evchan );
			adaptor->translate_ev( &ev, sdl_ev );
			if( NULL == chan 
			    || channelBlocked == try_write_CHAN( chan, adaptor->ev_size, &ev ) ) {
				
				// Bucket is full, drop event and print notice
				char buf[4096];	adaptor->detail_ev( &ev, sizeof(buf), buf );
				fprintf(stderr, "%s:%d: dropped event: (type: %d, time: %llu)\n", 
				        __FILE__, __LINE__,
				        type, ev.info.time);
				
			}
			
		}
		if( !(count > 0) )
			break;

		total = total + count;

	}

	return total;

}

bool quit_requested_EV( void ) {

	return quit_requested;

}

// Event channels /////////////////////////////////////////////////////////////

ev_channel_p open_EV( ev_adaptor_p adaptor, ... ) {

	enum ev_type_e type = adaptor->ev_type;
	ev_channel_p  evch = ev_channels[type];
	// Already open
	if( NULL != evch )
		return evch;

	// Initialize the device
	va_list args; va_start( args, adaptor );
	uint32 ev_mask = adaptor->init_ev( args );
	va_end(args);
	if( ev_mask != adaptor->ev_mask )
		return NULL;

	// Initialize a new channel
	const static int bufSize = 16;
	job_channel_p       sink = new_CHAN( adaptor->ev_size, bufSize );
	jobid           echo_job = nullJob;
	
	devices[type].params.source = sink;
	devices[type].params.ev_size = adaptor->ev_size;

	evch = new_EV_channel( sink );
	echo_job = submit_JOB( nullJob, 0, ioBound, NULL, (jobfunc_f)ev_echo, &devices[type].params );
		
	devices[type].sink = sink;
	devices[type].job = echo_job;
	ev_channels[type] = evch;

	// Enable the event mask
	ev_adaptors[type] = adaptor;
	SDL_ev_filter_mask |= ev_mask;

	return evch;
	
}
