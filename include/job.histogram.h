#ifndef __job_histogram_h__
#define __job_histogram_h__

#include "sync.condition.h"
#include "sync.mutex.h"

// Histogram //////////////////////////////////////////////////////////////////
// NOTE: These functions are not implicitly thread safe. It is up to the caller
//       to ensure calls to these functions are performed serially across
//       threads. In other words, callers should lock on a global object
//       before calling these.

int init_JOB_histogram( void );
int upd_JOB_histogram( uint32 deadline, int incr );
int wait_JOB_histogram( uint32 deadline, mutex_t* mutex, condition_t* signal );

#endif
