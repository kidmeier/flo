#include "ev.channel.h"
#include "core.alloc.h"

// Event channels

struct ev_channel_s {

	Channel*  sink;	

	uint16    capacity;
	uint16    top;
	Channel** stack;

};

Channel* peek_EV_sink( ev_channel_p evch ) {

	if( evch->top > 0 )
		return evch->stack[ evch->top-1 ];

	return NULL;

}

Channel* push_EV_sink( ev_channel_p evch, Channel* sink ) {
	
	Channel* top = peek_EV_sink( evch );
	if( evch->top >= evch->capacity ) {
		
		uint16 newCapacity = 2 * evch->capacity ;
		Channel** newStack = (Channel**)alloc( evch, 
		                                       newCapacity * sizeof(Channel*) );
		
		memcpy( newStack, evch->stack, evch->top * sizeof(Channel*) );

		delete(evch->stack);
		evch->capacity = newCapacity;
		evch->stack = newStack;

	}
	
	evch->stack[ evch->top++ ] = sink;
	evch->sink = sink;
	
	return top;
	
}

Channel* pop_EV_sink( ev_channel_p evch ) {
	
	if( evch->top > 0 ) {
		
		Channel* top = evch->stack[ --evch->top ];
		return top;
		
	}
	
	return NULL;
	
}

ev_channel_p new_EV_channel( Channel* sink ) {
	
	const static int initialCapacity = 4;
	ev_channel_p evch = new( NULL, ev_channel_t );
	
	evch->sink = NULL;
	
	evch->capacity = initialCapacity;
	evch->top = 0;
	evch->stack = (Channel**)alloc( NULL, initialCapacity*sizeof(Channel*) );
	memset( evch->stack, 0, initialCapacity*sizeof(Channel*) );
	
	if( sink )
		push_EV_sink(evch, sink);
	
	return evch;
}
