#include <assert.h>

#include "job.control.h"
#include "time.clock.h"
#include "time.core.h"
#include "core.alloc.h"

// Job
declare_job( int, clk_job, Clock* clk; Channel* control; float scale; Channel* sink );

// Data types

struct Clock {

	jobid    job;
	typeof_Job_params(clk_job) job_params;

	Channel* control;
	Channel* sink;

	uint     tick;
	float    step;
	
};

enum clkState_e {
   
	running,
	paused,
	stopped

};

struct Command {

	enum {
		
		clkReset,
		clkStart,
		clkStop,
		clkPause,
		clkTick,
		
	} tag;

	union {
		float scale;
		uint  tick;
	} arg;

};

#define clkCommandBuffer 4

define_job( int, clk_job,

            struct Command cmd;

            float      clk_time;
            enum clkState_e state;
            uint       tck;
            usec_t     tbase;
            usec_t     interval ) {

	begin_job;

	// Initial conditions
	local(clk_time) = arg(clk)->step * arg(clk)->tick;
	local(state)    = running;
	local(interval) = (usec_t)(arg(clk)->step / arg(scale) * usec_perSecond);

	// Send out t0
	writech( arg(sink), local(clk_time) );	

	// We track a contiguous ticks from the timebase. This enables us to
	// measure time from a fixed based, so that drift does not accumulate
	local(tck) = 1;
	local(tbase) = microseconds();
	while( !(stopped == local(state)) ) {

		// Read control packets and respond accordingly
		while( tryreadch( arg(control), local(cmd) ) ) {
		
			// Handle commands
			int tag = local(cmd).tag;
			if( clkReset == tag ) {
				
				arg(clk)->tick = local(cmd).arg.tick;

			} else if( clkStart == tag ) {

				local(state)    = running;
				local(interval) = (usec_t)
					(arg(clk)->step / local(cmd).arg.scale * usec_perSecond);

			} else if( clkStop == tag ) {
				
				local(state) = stopped;

			} else if( clkPause == tag 
			           || clkTick == tag ) {
				
				local(state) = paused;

			}

		}

		// If we've been paused, go to sleep until notified
		if( paused == local(state) ) {
						
			wait; // wait for start|tick notification
			
			// Rebase
			local(tck) = 1;
			local(tbase) = microseconds();
			
		}
		
		// Quit?
		if( stopped == local(state) )
			break;

		// Stay here until the next tick
		wait_until( local(tbase) + local(tck) * local(interval) < microseconds() );

		// Tick
		local(clk_time) = arg(clk)->step * (++arg(clk)->tick);
		local(tck)++;

		// Send tick
		writech( arg(sink), local(clk_time) );

	}

	end_job;

}

            

// C-/D-tors //////////////////////////////////////////////////////////////////

Clock* new_Clock( region_p R, float step, Channel* sink ) {

	assert( NULL != R );
	assert( step >= 0.f );
	assert( NULL != sink );

	Clock* clk = ralloc( R, sizeof(Clock) );
	
	clk->control = NULL;
	clk->sink    = sink;
	
	clk->tick    = 0;
	clk->step    = step;

	return clk;

}

void   delete_Clock( Clock* clk ) {

	stop_Clock( clk );

	if( clk->control )
		destroy_Channel( clk->control );

}

// Functions //////////////////////////////////////////////////////////////////

float  time_Clock( const Clock* clk ) {

	assert( NULL != clk );
	return timestep_Clock(clk) * ticks_Clock(clk);

}

uint   ticks_Clock( const Clock* clk ) {

	assert( NULL != clk );
	return clk->tick;

}

float  timestep_Clock( const Clock* clk ) {

	assert( NULL != clk );
	return clk->step;

}

// Mutators ///////////////////////////////////////////////////////////////////

int    set_Clock( Clock* clk, uint ticks ) {

	assert( NULL != clk );
	assert( NULL != clk->control ); {

		struct Command cmd = { .tag = clkReset, .arg.tick = 0 };
		return write_Channel( clk->job.job, clk->control, sizeof(cmd), &cmd );

	}

}

int    reset_Clock( Clock* clk ) {

	assert( NULL != clk );
	return set_Clock( clk, 0 );

}

int    start_Clock( Clock* clk, float scale ) {

	assert( NULL != clk );
	assert( 0.f != scale );

	struct Command cmd = { .tag = clkStart, .arg.scale = scale };
	if( NULL == clk->control ) {
		
		// Allocate channel
		clk->control = new_Channel( sizeof(cmd), clkCommandBuffer );
		assert( NULL != clk->control );
		
		// Submit the clock job
		clk->job_params.clk     = clk;
		clk->job_params.control = clk->control;
		clk->job_params.scale   = scale;
		clk->job_params.sink    = clk->sink;

		clk->job = submit_Job( 0,                   // Max priority
		                       ioBound, 
		                       NULL, 
		                       (jobfunc_f)clk_job, 
		                       &clk->job_params );
		return !jobBlocked;

	} else {

		int blocked = write_Channel( clk->job.job, clk->control, sizeof(cmd), &cmd );
		if( jobBlocked == blocked )
			return blocked;

		// Wakeup
		notify( clk->job );
		return !jobBlocked;

	}

}

int    stop_Clock( Clock* clk ) {

	assert( NULL != clk );
	assert( NULL != clk->control );

	struct Command stop = { .tag = clkStop };

	return write_Channel( clk->job.job, clk->control, sizeof(stop), &stop );

}

int    pause_Clock( Clock* clk ) {

	assert( NULL != clk );
	assert( NULL != clk->control );


	struct Command pause = { .tag = clkPause };

	return write_Channel( clk->job.job, clk->control, sizeof(pause), &pause );

}


int   tick_Clock( Clock* clk ) {

	assert( NULL != clk );
	assert( NULL != clk->control );

	struct Command tick = { .tag = clkTick };

	return write_Channel( clk->job.job, clk->control, sizeof(tick), &tick );

}

#ifdef __time_clock_TEST__

#include <stdio.h>
#include <stdlib.h>

#include "job.core.h"
#include "job.control.h"
#include "mm.heap.h"
#include "mm.region.h"
#include "sync.thread.h"

int sink_running = 1;

declare_job( int, clk_sink, Channel* source );
define_job( int, clk_sink, usec_t tbase; float ts; float realtime; float drift ) {

	begin_job;

	local(tbase) = microseconds();
	while( sink_running ) {

		readch( arg(source), local(ts) );
		local(realtime) = (float)(microseconds() - local(tbase)) / (float)usec_perSecond;
		printf(" ts =% 7.3fs  realtime =% 10.6fs  drift = %+9.6f  jitter = %+9.6f\n",
		       local(ts), local(realtime), local(realtime) - local(ts), 
		       local(drift) - (local(realtime) - local(ts)) );

		local(drift) = local(realtime) - local(ts);
	}

	end_job;

}

int main( int argc, char* argv[] ) {

	if( argc < 2 ) {
		fprintf(stderr, "Usage: %s <timestep>\n", argv[0]);
		return 1;
	}

	init_Jobs();
	
	float       step = strtof( argv[1], NULL );
	region_p       R = region( ZONE_heap, "time.clock.test");
	Channel*    sink = new_Channel( sizeof(float), 1 );
	Clock*       clk = new_Clock( R, step, sink );

	// Start the sink
	typeof_Job_params(clk_sink) params = { 
		sink 
	};
	submit_Job( 0, ioBound, NULL, (jobfunc_f)clk_sink, &params );

	// Start the clock
	start_Clock( clk, 1.f );

	// Wait 10 seconds
	sleep_THREAD( 4 * usec_perSecond );

	// Pause the clock
	pause_Clock( clk );
	printf("------------------------------------ PAUSE -----------------------------------\n");
	sleep_THREAD( 2 * usec_perSecond );

	start_Clock( clk, 1.f );

	sleep_THREAD( 4 * usec_perSecond );

	// Kill the sink
	sink_running = 0;

	// Stop the clock
	stop_Clock(clk);

	// Cleanup
	delete_Clock(clk);
	rfree( R );
	shutdown_Jobs();

}

#endif
