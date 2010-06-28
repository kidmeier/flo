#ifndef __core_control_h__
#define __core_control_h__

#include "core.seq.h"
#include "core.types.h"

/* Peform the 'body' once */
#define once(body) \ 
do {						\
  static int done = 0;				\
  if( !done )					\
    do { body } while(0);			\
  done = 1;					\
 } while(0)

#endif
