#ifndef __core_system_h__
#define __core_system_h__

#if defined(__linux__)

#include <unistd.h>

static inline
int cpu_count_SYS(void) {
	return (int)sysconf( _SC_NPROCESSORS_ONLN );
}

#else
#error "Unsupported platform"
#endif

#endif
