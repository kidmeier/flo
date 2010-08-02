#ifndef __sync_once_h__
#define __sync_once_h__

#include "core.features.h"

typedef void (*once_f)(void);

#if defined( feature_PTHREADS )

#include <pthread.h>
typedef pthread_once_t once_t;


#define init_ONCE	  \
	PTHREAD_ONCE_INIT

static inline
int do_ONCE( once_t* once, once_f func ) {

	return pthread_once( once, func );

}

#else
#error "Unsupported platform"
#endif

#define once( func )	  \
	do { \
		static once_t func##_once = init_ONCE; \
		do_ONCE( &func##_once, func ); \
	} while(0)

#endif
