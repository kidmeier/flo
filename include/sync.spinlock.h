#ifndef __sync_spinlock_h__
#define __sync_spinlock_h__

#include "core.features.h"

#if defined( feature_PTHREADS )

#include <pthread.h>
typedef pthread_spinlock_t spinlock_t;

static inline
int init_SPINLOCK( spinlock_t* lock ) {

	return pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE);

}

static inline
int destroy_SPINLOCK( spinlock_t* lock ) {

	return pthread_spin_destroy(lock);

}

static inline
int lock_SPINLOCK( spinlock_t* lock ) {

	return pthread_spin_lock(lock);

}

static inline
int unlock_SPINLOCK( spinlock_t* lock ) {

	return pthread_spin_unlock(lock);

}

static inline
int trylock_SPINLOCK( spinlock_t* lock ) {

	return pthread_spin_trylock( lock );

}

#else 

#error "Unsupported platform"

#endif 

#endif
