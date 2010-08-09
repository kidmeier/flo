#ifndef __ev_channel_h__
#define __ev_channel_h__

#include "ev.core.h"
#include "job.channel.h"

typedef struct ev_channel_s ev_channel_t;
typedef ev_channel_t* ev_channel_p;

ev_channel_p  new_EV_channel( job_channel_p sink );
job_channel_p peek_EV_sink( ev_channel_p evcha );
job_channel_p push_EV_sink( ev_channel_p evchan, job_channel_p chan );
job_channel_p pop_EV_sink( ev_channel_p evchan );

#endif
