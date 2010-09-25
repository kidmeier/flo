#ifndef __ev_channel_h__
#define __ev_channel_h__

#include "ev.core.h"
#include "job.channel.h"

typedef struct ev_channel_s ev_channel_t;
typedef ev_channel_t* ev_channel_p;

ev_channel_p  new_EV_channel( Channel* sink );
Channel*      peek_EV_sink( ev_channel_p evcha );
Channel*      push_EV_sink( ev_channel_p evchan, Channel* chan );
Channel*      pop_EV_sink( ev_channel_p evchan );

#endif
