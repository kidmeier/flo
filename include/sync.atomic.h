#ifndef __sync_atomic_h__
#define __sync_atomic_h__

#include "core.features.h"

#ifdef feature_GCC

#define atomic_cas( val, old, new ) \
	__sync_bool_compare_and_swap( &(val), (old), (new) )


#else
#error "Unsupported platform"
#endif

#endif
