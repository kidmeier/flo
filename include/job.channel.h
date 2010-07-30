#ifndef __job_channel_h__
#define __job_channel_h__

#include "core.types.h"
#include "job.core.h"
#include "sync.spinlock.h"

typedef struct job_channel_s job_channel_t;
typedef job_channel_t* job_channel_p;

#define channelEof     -1
#define channelBlocked -2

job_channel_p new_CHAN( uint16 size, uint16 count );
void          destroy_CHAN( job_channel_p chan );
int           write_CHAN( job_queue_p job, job_channel_p chan, uint16 size, pointer data );
int           read_CHAN( job_queue_p job, job_channel_p chan, uint16 size, pointer dest );

#endif
