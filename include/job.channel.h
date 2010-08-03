#ifndef __job_channel_h__
#define __job_channel_h__

#include "core.types.h"
#include "job.core.h"
#include "sync.spinlock.h"

typedef struct job_channel_s job_channel_t;
typedef job_channel_t* job_channel_p;
typedef struct job_chanalt_s job_chanalt_t;
typedef job_chanalt_t* job_chanalt_p;

#define channelRead     2
#define channelWrite    1
#define channelEof     -1
#define channelBlocked -2

job_channel_p new_CHAN( uint16 size, uint16 count );
void          destroy_CHAN( job_channel_p chan );
job_chanalt_p new_CHAN_alt( int n,
                            int alts[], 
                            job_channel_p channels[], 
                            uint16 sizes[], 
                            pointer ptrs[] );
void          destroy_CHAN_alt( job_chanalt_p chanalt );

int           write_CHAN( job_queue_p job, job_channel_p chan, uint16 size, pointer data );
int           read_CHAN( job_queue_p job, job_channel_p chan, uint16 size, pointer dest );

int           alt_CHAN( job_queue_p job, job_chanalt_p chanalt );
int           first_CHAN_alt( const job_chanalt_p chanalt );
int           next_CHAN_alt( const job_chanalt_p chanalt, int from );
pointer       data_CHAN_alt( const job_chanalt_p chanalt, int at );
uint16        size_CHAN_alt( const job_chanalt_p chanalt, int at );

#endif
