#ifndef __ev_channel_h__
#define __ev_channel_h__

#include "ev.core.h"
#include "job.channel.h"

typedef struct Ev_Channel Ev_Channel;

Ev_Channel *new_Ev_channel( Channel* sink );
Channel   *peek_Ev_sink( Ev_Channel *evchan );
Channel   *push_Ev_sink( Ev_Channel *evchan, Channel *chan );
Channel    *pop_Ev_sink( Ev_Channel *evchan );

#endif
