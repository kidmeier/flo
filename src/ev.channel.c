#include "ev.channel.h"
#include "core.alloc.h"

// Event channels

struct ev_channel_s {

	job_channel_p sink;
	
	uint16         capacity;
	uint16         top;
	job_channel_p* stack;

};

job_channel_p peek_EV_sink( ev_channel_p evch ) {

	if( evch->top > 0 )
		return evch->stack[ evch->top-1 ];

	return NULL;

}

job_channel_p push_EV_sink( ev_channel_p evch, job_channel_p sink ) {
	
	job_channel_p top = peek_EV_sink( evch );
	if( evch->top >= evch->capacity ) {

		uint16 newCapacity = 2 * evch->capacity ;
		job_channel_p* newStack = (job_channel_p*)alloc( evch, 
		                                                 newCapacity * sizeof(job_channel_p) );
		
		memcpy( newStack, evch->stack, evch->top * sizeof(job_channel_p) );

		delete(evch->stack);
		evch->capacity = newCapacity;
		evch->stack = newStack;

	}

	evch->stack[ evch->top++ ] = sink;
	evch->sink = sink;

	return top;

}

job_channel_p pop_EV_sink( ev_channel_p evch ) {

	if( evch->top > 0 ) {

		job_channel_p top = evch->stack[ --evch->top ];
		return top;

	}

	return NULL;

}

ev_channel_p new_EV_channel( job_channel_p sink ) {
	
	const static int initialCapacity = 4;
	ev_channel_p evch = new( NULL, ev_channel_t );

	evch->sink = NULL;

	evch->capacity = initialCapacity;
	evch->top = 0;
	evch->stack = (job_channel_p*)alloc( NULL, initialCapacity*sizeof(job_channel_p) );
	memset( evch->stack, 0, initialCapacity*sizeof(job_channel_p) );

	if( sink )
		push_EV_sink(evch, sink);

	return evch;
}

