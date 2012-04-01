#ifndef __sync_condition_h__
#define __sync_condition_h__

#include "core.features.h"
#include "core.types.h"
#include "sync.mutex.h"

#if defined( feature_PTHREADS ) || defined( feature_PTHREADS_W32 )

#include <pthread.h>
#include <sys/time.h>

typedef pthread_cond_t condition_t;

static inline
int init_CONDITION( condition_t* cond ) {

	*cond = (condition_t)PTHREAD_COND_INITIALIZER;
	return pthread_cond_init( cond, NULL );

}

static inline
int destroy_CONDITION( condition_t* cond ) {

	return pthread_cond_destroy( cond );

}

static inline
int signal_CONDITION( condition_t* cond ) {

	return pthread_cond_signal( cond );

}

static inline
int broadcast_CONDITION( condition_t* cond ) {

	return pthread_cond_broadcast( cond );

}

static inline
int wait_CONDITION( condition_t* cond, mutex_t* mutex ) {

	return pthread_cond_wait( cond, mutex );

}

static inline
int timed_wait_CONDITION( uint64 usec, condition_t* cond, mutex_t* mutex ) {
	
	long seconds = (long)(usec / 1000000ULL);
	long nsec = 1000L * (usec % 1000000ULL);
	struct timeval tv; gettimeofday( &tv, NULL );
	
	struct timespec ts;

	ts.tv_sec = tv.tv_sec + seconds;
	ts.tv_nsec = 1000L * tv.tv_usec + nsec;

	return pthread_cond_timedwait( cond, mutex, &ts );

}

#else

#error "Unsupported platform"

#endif

#endif
