#ifndef __core_system_h__
#define __core_system_h__

#include "core.features.h"

#if defined( feature_POSIX )

#include <unistd.h>

static inline
int cpu_count_SYS(void) {
	return (int)sysconf( _SC_NPROCESSORS_ONLN );
}

#else
#error "Unsupported platform"
#endif

#endif
