#ifndef __sync_mutex_h__
#define __sync_mutex_h__

#include "core.features.h"

#if defined( feature_PTHREADS )

#include <pthread.h>
typedef pthread_mutex_t mutex_t;

static inline
int init_MUTEX( mutex_t* mutex ) {

	*mutex = (mutex_t)PTHREAD_MUTEX_INITIALIZER;
	return pthread_mutex_init( mutex, NULL );

}

static inline
int destroy_MUTEX( mutex_t* mutex ) {

	return pthread_mutex_destroy(mutex);

}

static inline
int lock_MUTEX( mutex_t* mutex ) {

	return pthread_mutex_lock( mutex );

}

static inline
int unlock_MUTEX( mutex_t* mutex ) {

	return pthread_mutex_unlock( mutex );

}

#else 

#error "Unsupported platform"

#endif

#endif
