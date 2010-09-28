#ifndef __job_channel_h__
#define __job_channel_h__

#include "core.types.h"
#include "job.core.h"
#include "sync.spinlock.h"

typedef struct Channel Channel;
typedef struct Chanmux Chanmux;

typedef enum {

	channelRead    =  2,
	channelWrite   =  1,

} muxOp_e;

#define channelEof     -1
#define channelBlocked -2

Channel*       new_Channel( uint16 size, uint16 count );
void       destroy_Channel( Channel* chan );

int       try_read_Channel( Channel* chan, uint16 size, pointer dest );
int      try_write_Channel( Channel* chan, uint16 size, pointer data );
int          write_Channel( Job* job, Channel* chan, uint16 size, pointer data );
int           read_Channel( Job* job, Channel* chan, uint16 size, pointer dest );

void         flush_Channel( Channel* chan );
void          poll_Channel( Channel* chan );

// Channel muxing
int            mux_Channel( Job* job, Chanmux* chanalt );

Chanmux*       new_Chanmux( int      n,
                            muxOp_e  ops[], 
                            Channel* channels[], 
                            uint16   sizes[], 
                            pointer  ptrs[] );
void       destroy_Chanmux( Chanmux* chanalt );
int          first_Chanmux( const Chanmux* chanalt );
int           next_Chanmux( const Chanmux* chanalt, int from );
pointer       data_Chanmux( const Chanmux* chanalt, int at );
uint16        size_Chanmux( const Chanmux* chanalt, int at );

#endif
