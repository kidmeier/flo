#ifndef __seq_filter_h__
#define __seq_filter_h__

#include "seq.core.h"
extern seqable_p filter_SEQ;

#define NEXT_FILTER_SEQ 0
#define REST_FILTER_SEQ 1

seq_t filter( seq_t sq, bool (*pred)(any), int how );

#endif
