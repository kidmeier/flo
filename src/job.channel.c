#include <assert.h>

#include "data.list.h"
#include "data.ringbuf.h"
#include "job.channel.h"
#include "job.queue.h"
#include "mm.region.h"

struct Channel {

	region_p   R;
	spinlock_t lock;

	List*      readq;
	List*      writeq;

	Chanmux*   mux_read;
	Chanmux*   mux_write;

	ringbuf_p  ring;

};

struct Chanmux {

	region_p   R;
	spinlock_t lock;

	List*      waitq;

	int        N;
	
	muxOp_e*   ops;
	Channel**  channels;
	uint16*    sizes;
	pointer*   ptrs;

	int*       stati;

};

// Primitive ops

static void flush( spinlock_t* lock, Channel* chan ) {

	wakeup_waitqueue_Job( NULL, &chan->readq );
	// If there are any muxes, wake them up
	if( chan->mux_read ) 
		wakeup_waitqueue_Job( &chan->mux_read->lock, &chan->mux_read->waitq );
	
}

static void poll( spinlock_t* lock, Channel* chan ) { 
	
	wakeup_waitqueue_Job( lock, &chan->writeq );
	if( chan->mux_write ) 
		wakeup_waitqueue_Job( &chan->mux_write->lock, &chan->mux_write->waitq );

}

// Try to write `size` bytes to channel from 'data`; returns channelBlocked if
// not enough bytes left in buffer; returns `size` on success
//
// Assume `chan` is appropriately locked by caller
static int try_write( Channel* chan, uint16 size, pointer data ) {

	int ret = write_RINGBUF(chan->ring, size, data);
	if( ret < 0 ) {
		flush( NULL, chan ); // Force a flush since we're full
		return channelBlocked;
	}

	// Automatic flush once we've filled the buffer
	if( 0 == remaining_RINGBUF(chan->ring) )
		flush( NULL, chan );

	return ret;

}

// Try to read `size` bytes from channel into 'dest`; returns channelBlocked if
// not enough bytes are available for reading; returns `size` on success
//
// Assume `chan` is appropriately locked by caller
static int try_read( Channel* chan, uint16 size, pointer dest ) {

	int ret = read_RINGBUF( chan->ring, size, dest );
	if( ret < 0 ) {
		poll( NULL, chan ); // Poll writers, we're empty
		return channelBlocked;
	}

	// Automatic poll once we've consumed the buffer
	if( 0 == available_RINGBUF(chan->ring) )
		poll( NULL, chan );

	return ret;

}

// Public API /////////////////////////////////////////////////////////////////

Channel* new_Channel( uint16 size, uint16 count ) {

	region_p R = region( "job.channel::new_Channel" );
//	Channel* chan = new( NULL, Channel );
	Channel* chan = ralloc( R, sizeof(Channel) );

	if( init_SPINLOCK(&chan->lock) ) {
		rfree( R );
		return NULL;
	}
	chan->R = R;

	chan->readq = new_List( R, sizeof(Job) );
	chan->writeq = new_List( R, sizeof(Job) );

	chan->mux_read = NULL;
	chan->mux_write = NULL;

	chan->ring = new_RINGBUF( size, count );
	if( !chan->ring ) {
		destroy_SPINLOCK(&chan->lock);
		rfree( chan->R );

		return NULL;
	}

	return chan;

}

void          destroy_Channel( Channel* chan ) {

	// TODO: What action to take if there are blocked jobs on chan?
	destroy_SPINLOCK( &chan->lock );
	destroy_RINGBUF( chan->ring );
	rfree( chan->R );

}

int           write_Channel( Job* job, Channel* chan, uint16 size, pointer data ) {

	lock_SPINLOCK( &chan->lock );

	int ret = try_write( chan, size, data );
	if( channelBlocked == ret ) {
		sleep_waitqueue_Job( NULL, &chan->writeq, job );
	}

	unlock_SPINLOCK( &chan->lock );

	return ret;
}

int           try_write_Channel( Channel* chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );
	int ret = try_write( chan, size, dest );
	unlock_SPINLOCK( &chan->lock );
	
	return ret;
	
}

int           read_Channel( Job* job, Channel* chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );

	int ret = try_read( chan, size, dest );
	if( channelBlocked == ret ) {
		sleep_waitqueue_Job( NULL, &chan->readq, job );
	}

	unlock_SPINLOCK( &chan->lock );

	return ret;
}

int try_read_Channel( Channel* chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );
	int ret = try_read( chan, size, dest );
	unlock_SPINLOCK( &chan->lock );
	
	return ret;
	
}

void flush_Channel( Channel* chan ) {

	// When called through public interface we pass the lock
	flush( &chan->lock, chan );

}

void poll_Channel( Channel* chan ) {

	// When called through public interface we pass the lock
	poll( &chan->lock, chan );

}

int mux_Channel( Job* job, Chanmux* mux ) {

	int ready = 0;

	lock_SPINLOCK( &mux->lock );
	for( int i=0; i<mux->N; i++ ) {
		
		muxOp_e     op = mux->ops[i];
		Channel*  chan = mux->channels[i];
		uint16    size = mux->sizes[i];
		pointer    ptr = mux->ptrs[i];
		int*    status = &mux->stati[i];
		
		Chanmux** chan_mux;
		Chanmux** chan_mux_other;
		int (*try_alt)( Channel*, uint16, pointer );

		lock_SPINLOCK( &chan->lock );
		switch( op ) {
		case channelRead:
			chan_mux = &chan->mux_read;
			chan_mux_other = &chan->mux_write;
			try_alt = try_read_Channel;
			break;
		case channelWrite:
			chan_mux = &chan->mux_write;
			chan_mux_other = &chan->mux_read;
			try_alt = try_write_Channel;
			break;
		}
		
		// Can only be involved in one mux at a time; either us or nobody
		assert( NULL == *(chan_mux) || mux == *(chan_mux) );
		// We can't wait on a read and a write in the same alt
		assert( NULL == *(chan_mux_other) || mux != *(chan_mux_other) );

		*(status) = (try_alt)( chan, size, ptr );

		// Success; clear the mux from the channel and inc our `ready` counter
		if( channelBlocked != *(status) ) {

			*(chan_mux) = NULL;
			ready++;
			
		} else 
			// Would block; mark ourself as waiting in the channel's appropriate `alt`
			*(chan_mux) = mux;

		unlock_SPINLOCK( &chan->lock );
		
	}
	
	// If nothing is ready tell the caller to block
	if( !ready ) {
		sleep_waitqueue_Job( NULL, &mux->waitq, job );
		ready = channelBlocked;
	}
	unlock_SPINLOCK( &mux->lock );

	return ready;
}

Chanmux* new_Chanmux( int      n, 
                      muxOp_e  ops[], 
                      Channel* channels[], 
                      uint16   sizes[], 
                      pointer  ptrs[] ) {
	
	region_p R = region( "job.channel::new_Channel" );
	Chanmux* mux = ralloc( R, sizeof(Chanmux) );
	
	if( init_SPINLOCK( &mux->lock ) < 0 ) {
		rfree(R);
		return NULL;
	}
	mux->R     = R;
	mux->waitq = new_List( R, sizeof(Job) );
	
	mux->N = n;
	
	mux->ops = ralloc( R, n * sizeof(muxOp_e) );
	mux->channels = ralloc( R, n * sizeof(Channel*) );
	mux->sizes = ralloc( R, n * sizeof(uint16) );
	mux->ptrs = ralloc( R, n * sizeof(pointer));
	
	mux->stati = ralloc( R, n * sizeof(int) );

#ifdef DEBUG
	for( int i=0; i<n; i++ ) {
		assert( NULL == channels[i]->mux_read 
		        && NULL == channels[i]->mux_write );
	}
#endif
	
	memcpy( mux->ops, ops, n * sizeof(int) );
	memcpy( mux->channels, channels, n * sizeof(Channel*) );
	memcpy( mux->sizes, sizes, n * sizeof(uint16) );
	memcpy( mux->ptrs, ptrs, n * sizeof(pointer) );
	memset( mux->stati, 0, n * sizeof(int) );

	return mux;

}

void          destroy_Chanmux( Chanmux* mux ) {

	destroy_SPINLOCK( &mux->lock );
	rfree(mux->R);

}

int           first_Chanmux( const Chanmux* mux ) {

	for( int i=0; i<mux->N; i++ ) {
		if( mux->stati[i] > 0 )
			return i;
	}

	return channelEof;

}

int           next_Chanmux( const Chanmux* mux, int from ) {

	for( int i=from; i<mux->N; i++ ) {
		if( mux->stati[i] > 0 )
		    return i;
	}

	return channelEof;

}

pointer       data_Chanmux( const Chanmux* mux, int at ) {

	assert( at >= 0 && at < mux->N );
	return mux->ptrs[at];

}

uint16        size_Chanmux( const Chanmux* mux, int at ) {

	assert( at >= 0 && at < mux->N );
	return mux->sizes[at];

}

#ifdef __job_channel_TEST__

#include <stdio.h>
#include "job.control.h"
#include "job.core.h"

declare_job( int, producer, Channel* out );
declare_job( int, consumer, Channel* in );

define_job( int, producer, int I ) {

	begin_job;

	local( I ) = 0;
	while( 1 ) {

		writech( arg(out), local(I) );
		fprintf(stdout, "produced: %d\n", local(I) );
		
		local(I)++;

	}

	end_job;
}

define_job( int, consumer, int I ) {

	begin_job;

	while( 1 ) {

		readch( arg(in), local(I) );
		fprintf(stdout, "consumed: %d\n", local(I) );

	}

	end_job;
}

int main(int argc, char* argv[] ) {

	if( argc < 2 ) {
		fprintf(stderr, "usage: %s <n_workers>\n", argv[0]);
		return 1;
	}

	int n_threads = (int)strtol( argv[1], NULL, 10 );
	init_Jobs( n_threads );

	// Make the channel size not a multiple of sizeof(int)
	// this ensures we exercise more code paths.
	Channel* chan = new_Channel( sizeof(char), 13 );

	int c_ret;
	typeof_Job_params(consumer) c_params = { chan };

	int p_ret;
	typeof_Job_params(producer) p_params = { chan };

	jobid cons = submit_Job( 0, ioBound, &c_ret, (jobfunc_f)consumer, &c_params);
	jobid prod = submit_Job( 0, ioBound, &p_ret, (jobfunc_f)producer, &p_params);

	mutex_t mutex; init_MUTEX(&mutex);
	condition_t cond; init_CONDITION(&cond);

	fprintf(stdout, "jobs submitted; watch the magic\n");

	lock_MUTEX(&mutex);
	while( join_deadline_Job( 0, &mutex, &cond ) < 0 );
	unlock_MUTEX(&mutex);

	destroy_Channel(chan);
	shutdown_Jobs();

	return 0;
}

#endif
