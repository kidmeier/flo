#include <assert.h>
#include <SDL_events.h>

#include "core.log.h"
#include "core.types.h"

#include "ev.core.h"
#include "ev.channel.h"

#include "job.control.h"

// Forward decls
declare_job( void, ev_echo, Channel* source; int ev_size );

// SDL data wrangling

static uint8 SDL_enable_ev( uint32 ev_type ) {

	return SDL_EventState( ev_type, SDL_ENABLE );

}

static uint8 SDL_disable_ev( uint32 ev_type ) {

	return SDL_EventState( ev_type, SDL_IGNORE );

}

static enum ev_type_e SDL_ev_type( const SDL_Event* ev ) {

	switch( ev->type ) {

	case SDL_WINDOWEVENT:
		switch( ev->window.event )
		{
		case SDL_WINDOWEVENT_ENTER:
		case SDL_WINDOWEVENT_LEAVE:
		case SDL_WINDOWEVENT_FOCUS_GAINED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			return evFocus;

		case SDL_WINDOWEVENT_MINIMIZED:
		case SDL_WINDOWEVENT_MAXIMIZED:
		case SDL_WINDOWEVENT_RESTORED:
		case SDL_WINDOWEVENT_MOVED:
		case SDL_WINDOWEVENT_RESIZED:
		case SDL_WINDOWEVENT_SHOWN:
		case SDL_WINDOWEVENT_HIDDEN:
		case SDL_WINDOWEVENT_EXPOSED:
		case SDL_WINDOWEVENT_CLOSE:
			return evWindow;
		}

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		return evKeyboard;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		return evButton;

	case SDL_JOYAXISMOTION:
		return evAxis;

	case SDL_MOUSEMOTION:
	case SDL_JOYBALLMOTION:
		return evCursor;

	case SDL_JOYHATMOTION:
		return evDpad;

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

	// We always listen for quit
	SDL_EventState( SDL_QUIT, SDL_ENABLE );

	// Future?
	SDL_EventState( SDL_CLIPBOARDUPDATE, SDL_IGNORE );
	SDL_EventState( SDL_CLIPBOARDUPDATE, SDL_IGNORE );
	SDL_EventState( SDL_CLIPBOARDUPDATE, SDL_IGNORE );
}

// Internal data //////////////////////////////////////////////////////////////

struct ev_device_s {

	Channel*                   sink;
	Handle                     job;
	typeof_Job_params(ev_echo) params;

};

static ev_adaptor_p       ev_adaptors[evTypeCount];
static struct ev_device_s devices    [evTypeCount];
static Ev_Channel        *ev_channels[evTypeCount];

static msec_t             base_ev_time   = 0;
static bool               quit_requested = false;

static ev_adaptor_p get_adaptor( const ev_t* ev ) {

	return ev_adaptors[ev->info.type];

}

static int detail_ev( const ev_t* ev, int maxlen, char* dst ) {

	ev_adaptor_p adaptor = get_adaptor(ev);
	return adaptor->detail_ev( ev, maxlen, dst );

}

// Root event handler /////////////////////////////////////////////////////////

// Echo job; This is the base-level `sink` installed at the bottom of each 
// Ev_Channel *. It simply echoes the `detail` string of the event to stdout.
define_job( void, ev_echo, 

            ev_t ev;
            char ev_desc[4092] ) {

	begin_job;
	
	while(1) {

		readch_buf( arg(source), arg(ev_size), &local(ev) );

		if( detail_ev( &local(ev), sizeof(local(ev_desc)), local(ev_desc) ) > 0 )
			trace( "% 8.4fs %s", 
			       (double)local(ev).info.time / usec_perSecond, 
			       local(ev_desc) );
		
	}
	
	end_job;
}

// Public API /////////////////////////////////////////////////////////////////

int init_Ev( void ) {

	memset( &ev_channels, 0, sizeof(ev_channels) );
	memset( &devices, 0, sizeof(devices) );
	memset( &ev_adaptors, 0, sizeof(ev_adaptors) );
	init_SDL_ev();

	quit_requested = false;
	base_ev_time = microseconds();
	return 0;

}

void wait_Ev( void ) {

	SDL_WaitEvent( NULL );

}

int pump_Ev( uint32 tick ) {

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

		int count = SDL_PeepEvents(&events[0], numEvents, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
		for( int i=0; i<count; i++ ) {

			const SDL_Event* sdl_ev = &events[i];
			enum ev_type_e     type = SDL_ev_type(sdl_ev);
			ev_adaptor_p    adaptor = ev_adaptors[type];
			Ev_Channel      *evchan = ev_channels[type];
			ev_t ev;

			// Check for QUIT and flag it
			if( SDL_QUIT == sdl_ev->type ) {

				quit_requested = true;
				// Application is not listening for SDL_QUIT, skip
				if( NULL == evchan )
					continue;

			} else {
				// SDL 1.3 likes to send us events we don't understand!?
				if( evUnknown == type ) {
					count--;
					continue;
				}
			}
			
			assert( NULL != evchan );
			assert( NULL != adaptor );

			// Stamp the event
			ev.info.time = microseconds() - base_ev_time;
			ev.info.tick = tick;
			ev.info.type = type;

			// Translate it
			adaptor->translate_ev( &ev, sdl_ev );

			// Dispatch
			Channel* chan = peek_Ev_sink( evchan );
			if( NULL == chan
			 || channelBlocked == try_write_Channel( chan, 
			                                         adaptor->ev_size,
			                                         &ev ) ) {
				
				// Bucket is full, drop event and print notice
				char buf[4096];	adaptor->detail_ev( &ev, sizeof(buf), buf );

				warning("dropped event: (type: %d, time: %llu)",
				        type, 
				        ev.info.time);
				
			} else {

				flush_Channel(chan);

			}
			
		}
		if( !(count > 0) )
			break;

		total = total + count;

	}

	return total;

}

bool quit_Ev_requested( void ) {

	return quit_requested;

}

// Event channels /////////////////////////////////////////////////////////////

Ev_Channel *open_Ev( ev_adaptor_p adaptor, ... ) {

	enum ev_type_e type = adaptor->ev_type;
	Ev_Channel    *evch = ev_channels[type];
	// Already open
	if( NULL != evch )
		return evch;

	// Initialize the device (if needed)
	va_list args; va_start( args, adaptor );
	int ret = adaptor->init_ev( SDL_enable_ev, SDL_disable_ev, args );
	va_end(args);
	if( ret < 0 )
		return NULL;

	// Initialize a new channel
	static const int bufSize = 16;
	Channel*         sink    = new_Channel( adaptor->ev_size, bufSize );
	Handle           echo_job;
	devices[type].params.source  = sink;
	devices[type].params.ev_size = adaptor->ev_size;

	evch = new_Ev_channel( sink );
	echo_job = submit_Job( (uint32)-1, ioBound, NULL, (jobfunc_f)ev_echo, &devices[type].params );
		
	devices[type].sink = sink;
	devices[type].job = echo_job;
	ev_channels[type] = evch;

	// Enable the event mask
	ev_adaptors[type] = adaptor;

	return evch;
	
}

void        close_Ev( Ev_Channel *evch ) {

	warning0( "close_Ev(): Not yet implemented" );

}
