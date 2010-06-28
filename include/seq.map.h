#ifndef __seq_map_h__
#define __seq_map_h__

#include "seq.core.h"
declare_SEQABLE(map);

seq_t map( any (*f)(any), seq_t sq );

#endif

