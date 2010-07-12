#ifndef __time_core_h__
#define __time_core_h__

#include "core.types.h"

typedef uint64 msec_t;
typedef uint64 usec_t;

#if defined(__linux__)

#include <stdlib.h>
#include <sys/time.h>

static inline msec_t milliseconds() {

	struct timeval tv; gettimeofday(&tv, NULL);
	return 1000ULL*tv.tv_sec + tv.tv_usec / 1000;

}

static inline usec_t microseconds() {

	struct timeval tv; gettimeofday(&tv, NULL);
	return 1000000ULL*tv.tv_sec + tv.tv_usec;

}

#else
#error "Unsupported platform"
#endif

#endif
