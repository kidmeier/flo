#include "ev.channel.h"

// Event channels

struct Ev_Channel {

	Channel*  sink;	

	uint16    capacity;
	uint16    top;
	Channel** stack;

};

Channel* peek_Ev_sink( Ev_Channel *evch ) {

	if( evch->top > 0 )
		return evch->stack[ evch->top-1 ];

	return NULL;

}

Channel* push_Ev_sink( Ev_Channel *evch, Channel* sink ) {
	
	Channel* top = peek_Ev_sink( evch );
	if( evch->top >= evch->capacity ) {
		
		uint16 newCapacity = 2 * evch->capacity ;
		Channel** newStack = calloc( newCapacity, sizeof(Channel*) );
		
		memcpy( newStack, evch->stack, evch->top * sizeof(Channel*) );

		free(evch->stack);
		evch->capacity = newCapacity;
		evch->stack = newStack;

	}
	
	evch->stack[ evch->top++ ] = sink;
	evch->sink = sink;
	
	return top;
	
}

Channel* pop_Ev_sink( Ev_Channel *evch ) {
	
	if( evch->top > 0 ) {
		
		Channel* top = evch->stack[ --evch->top ];
		return top;
		
	}
	
	return NULL;
	
}

Ev_Channel *new_Ev_channel( Channel* sink ) {
	
	const static int initialCapacity = 4;
	Ev_Channel *evch = malloc( sizeof(Ev_Channel) );
	
	evch->sink = NULL;
	
	evch->capacity = initialCapacity;
	evch->top = 0;
	evch->stack = calloc( initialCapacity, sizeof(Channel*) );
	memset( evch->stack, 0, initialCapacity*sizeof(Channel*) );
	
	if( sink )
		push_Ev_sink(evch, sink);
	
	return evch;
}
