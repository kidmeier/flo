#ifndef __phys_clock_h__
#define __phys_clock_h__

#include "core.types.h"
#include "job.channel.h"
#include "mm.region.h"

typedef struct Clock Clock;

// Clock //////////////////////////////////////////////////////////////////////
//
// A Clock simulates a discrete time source. When running, the clock increments
// its tick count for every `step` seconds of elapsed real-time. Additionally,
// when a tick occurs, the current clock time is evaluated and written to the 
// `sink`.
//
// The clock time is evaluated by multiplying the tick count by the `step`
// parameter. The ratio between the passage of clock-time to real-time can 
// be manipulated using the `scale` parameter to the `start_Clock` routine.

// C-/D-tors //////////////////////////////////////////////////////////////////

// Allocate memory for a clock
//
// @R - region from which to allocate memory
Clock* alloc_Clock( region_p R );

// Initialize `clk` with a timestep of `step` that writes to `sink`
//
// @clk  - Clock* to initialize
// @step - Clock's timestep
// @sink - The channel upon which to write discrete time signals
Clock*  init_Clock( Clock* clk, float step, Channel* sink );

// Create a new clock with step size `step` that writes to `sink`
//
// @R - Region from which to allocate the clock
// @step - Step size, in seconds; must be >= 0.f
// @sink - The channel upon which to write discrete time signals
Clock* new_Clock( region_p R, float step, Channel* sink );

// Destroy clock. Clock is first stopped if its running. Memory is
// reclaimed when the region from which the clock was allocated is
// collected.
//
// @clk - Clock to delete
void   delete_Clock( Clock* clk );

// Functions //////////////////////////////////////////////////////////////////

// Equivalent to: ticks_Clock(clk) * timestep_Clock(clk)
//
// @clk - Clock* to measure
// @return - the clock's time, as measured by whole units of the timestep
float  time_Clock( const Clock* clk );

// Return the number of ticks in this clock's counter
//
// @clk - Clock* to measure
// @return - the number of ticks this clock has taken since the last re/set
uint   ticks_Clock( const Clock* clk );

// The amount of time said to be covered during each tick
//
// @clk - Clock* to measure
// @return - the clock's timestep
float  timestep_Clock( const Clock* clk );

// Mutators ///////////////////////////////////////////////////////////////////

// NOTE:
//
// These functions can all be called from within a job. The return value of
// each is a job status to be used for job control to indicate whether or
// not to block the calling job.
//
// This means that if used outside of a job, the return value must
// be checked and the operation must be polled until it succeeds.

// Set the clock's tick count.
//
// @clk - Clock* to set
// @ticks - time to set
int    set_Clock( Clock* clk, uint ticks );

// Reset the clock's tick count to 0
//
// @clk - Clock* to reset
int    reset_Clock( Clock* clk );

// Start the clock running. The clock will send the current time on the `sink`
// channel for every timestep_Clock(clk) / scale seconds that pass in real world 
// time. This constrains `scale >= 0.0`.
//
// @clk - Clock* to start
// @scale - Ratio of clock-world time to real-time. For example, if scale
//          is 2.f, and `step` is 1.0f, then 1 second of clock time will
//          elapse every 0.5 seconds of real-time.
int    start_Clock( Clock* clk, float scale );

// Stop the clock. Implicitly performs a reset and also removes the clock's 
// timing job from the job queue. Hence starting is marginally more expensive
// after calling this.
//
// @clk - Clock* to stop
int    stop_Clock( Clock* clk );

// Stop the clock from sending time signals to the `sink` channel. From here individual
// steps can be taken with `tick_Clock`.
//
// @clk - Clock* to pause
int    pause_Clock( Clock* clk );

// Advance the clock by one tick signaling the next time on the `sink` channel.
// If the clock is not paused, this has the effect of pausing it.
//
// @clk - Clock* to tick
int    tick_Clock( Clock* clk );

#endif
