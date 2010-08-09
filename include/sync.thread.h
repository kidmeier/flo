#ifndef __sync_thread_h__
#define __sync_thread_h__

#include "core.features.h"
#include "time.core.h"

typedef int (*threadfunc_f)( void* arg );

#if defined( feature_PTHREADS ) 

#include <pthread.h>
typedef pthread_t thread_t;

static inline
int create_THREAD( thread_t* t, threadfunc_f run, void* arg ) {

	return pthread_create( t, NULL, (void * (*)(void *))run, arg );

}

static inline
int join_THREAD( thread_t* t, int* ret ) {

	return pthread_join( *t, (void**)ret );

}

static inline
thread_t self_THREAD(void) {

	return pthread_self();

}

static inline
int yield_THREAD(void) {

	return pthread_yield();

}

static inline
int sleep_THREAD( usec_t usec ) {

	struct timespec ts = { 
		.tv_sec = (time_t)(usec / 1000000ULL),
		.tv_nsec = 1000L * (usec % 1000000ULL)
	};

	return nanosleep( &ts, NULL );

}

static inline
int cancel_THREAD( thread_t* t ) {

	return pthread_cancel( *t );

}

static inline
void exit_if_cancelled_THREAD(void) {

	pthread_testcancel();

}

#else
#error "Unsupported platform"
#endif

#endif // __sync_thread_h__
