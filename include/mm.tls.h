#ifndef __mm_tls_h__
#define __mm_tls_h__

#include "core.features.h"
#include "core.types.h"

#ifndef feature_TLS
#error  This module requires compiler thread-local storage support. Maybe fix core.features.h?
#endif

// Thread local storage 
//
// Implements thread local stack storage area. Purpose is to provide
// _fast_ dynamic allocation for use within a single function. Note
// that for jobs to safely use this, memory must be allocated and freed a
// during an uninterruptable section. 
//
// In other words, memory must be freed within same non-blocking region
// of code that it is allocated in. Additionally in must be freed in
// the reverse order it was allocated. Its a manual stack :/.
//
// This gives a somewhat satisfactory portable implementation of `alloca`.
// At least to the degree that TLS is portable (it is by now, right?)

// Allocate `size` bytes on the thread-local stack
pointer pushtls( uint size );

// Pop `size` bytes off of the stack
void    poptls( pointer ptr );

#endif
