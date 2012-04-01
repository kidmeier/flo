#ifndef __time_core_h__
#define __time_core_h__

#include "core.features.h"
#include "core.types.h"

#define msec_perSecond       1000ULL
#define usec_perSecond    1000000ULL
#define nsec_perSecond 1000000000ULL

typedef uint64 msec_t;
typedef uint64 usec_t;

#if defined( feature_POSIX ) || defined( feature_MINGW )

#include <stdlib.h>
#include <sys/time.h>

static inline msec_t milliseconds() {

	struct timeval tv; gettimeofday(&tv, NULL);
	return msec_perSecond*tv.tv_sec + tv.tv_usec / 1000;

}

static inline usec_t microseconds() {

	struct timeval tv; gettimeofday(&tv, NULL);
	return usec_perSecond*tv.tv_sec + tv.tv_usec;

}

#else
#error "Unsupported platform"
#endif

#endif
