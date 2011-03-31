#include <assert.h>
#include <string.h>

#include "core.log.h"
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

	ringbuf_p  ring;

};

struct Chanmux {

	region_p   R;
	spinlock_t lock;

	int        N;
	
	muxOp_e*   ops;
	Channel**  channels;
	uint16*    sizes;
	pointer*   ptrs;

	int*       stati;

};

// Primitive ops

static void flush( spinlock_t* lock, Channel* chan ) {

	wakeup_waitqueue_Job( lock, &chan->readq );
	
}

static void poll( spinlock_t* lock, Channel* chan ) { 
	
	wakeup_waitqueue_Job( lock, &chan->writeq );

}

// Try to write `size` bytes to channel from 'data`; returns channelBlocked if
// not enough bytes left in buffer; returns `size` on success
//
// Assume `chan` is appropriately locked by caller
static int try_write( Channel* chan, uint16 size, const pointer data ) {

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

Channel*       new_Channel( uint16 size, uint16 count ) {

	region_p R = region( "job.channel::new_Channel" );
	Channel* chan = ralloc( R, sizeof(Channel) );

	if( init_SPINLOCK(&chan->lock) ) {
		rfree( R );
		return NULL;
	}
	chan->R = R;

	chan->readq = new_List( R, sizeof(Job) );
	chan->writeq = new_List( R, sizeof(Job) );

	chan->ring = new_RINGBUF( size, count );
	if( !chan->ring ) {
		destroy_SPINLOCK(&chan->lock);
		rfree( chan->R );

		return NULL;
	}

	return chan;

}

void       destroy_Channel( Channel* chan ) {

	// TODO: What action to take if there are blocked jobs on chan?
	destroy_SPINLOCK( &chan->lock );
	destroy_RINGBUF( chan->ring );
	rfree( chan->R );

}

int          write_Channel( Job* job, Channel* chan, uint16 size, const pointer data ) {

	lock_SPINLOCK( &chan->lock );

	int ret = try_write( chan, size, data );
	if( channelBlocked == ret )
		sleep_waitqueue_Job( NULL, &chan->writeq, job );

	unlock_SPINLOCK( &chan->lock );

	return ret;
}

int      try_write_Channel( Channel* chan, uint16 size, const pointer data ) {

	lock_SPINLOCK( &chan->lock );
	int ret = try_write( chan, size, data );
	unlock_SPINLOCK( &chan->lock );
	
	return ret;
	
}

int           read_Channel( Job* job, Channel* chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );

	int ret = try_read( chan, size, dest );
	if( channelBlocked == ret )
		sleep_waitqueue_Job( NULL, &chan->readq, job );

	unlock_SPINLOCK( &chan->lock );

	return ret;
}

int       try_read_Channel( Channel* chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );
	int ret = try_read( chan, size, dest );
	unlock_SPINLOCK( &chan->lock );
	
	return ret;
	
}

void         flush_Channel( Channel* chan ) {

	// When called through public interface we pass the lock
	flush( &chan->lock, chan );

}

void          poll_Channel( Channel* chan ) {

	// When called through public interface we pass the lock
	poll( &chan->lock, chan );

}

int            mux_Channel( Job* job, Chanmux* mux ) {

	int ready = 0;

	lock_SPINLOCK( &mux->lock );

	// Release the job lock while we acquire these locks
	unlock_SPINLOCK( &job->lock );

	// First lock all the channels
	for( int i=0; i<mux->N; i++ ) {
		lock_SPINLOCK( &mux->channels[i]->lock );
	}

	// Grab it back before we start waking shit up
	lock_SPINLOCK( &job->lock );

	for( int i=0; i<mux->N; i++ ) {
		
		muxOp_e     op = mux->ops[i];
		Channel*  chan = mux->channels[i];
		uint16    size = mux->sizes[i];
		pointer    ptr = mux->ptrs[i];
		int*    status = &mux->stati[i];
		
		int (*try_op)( Channel*, uint16, pointer ) = NULL;

		switch( op ) {
		case channelRead:
			try_op = try_read;
			break;
		case channelWrite:
			try_op = try_write;
			break;
		default:
			// Panic!
			fatal( "Unknown channel op: %d", op );
			break;
		}

		// Try the read/write		
		*(status) = (try_op)( chan, size, ptr );

		// Success!
		if( channelBlocked != *(status) )
			ready++;
		
	}

	// If nothing is ready, wait on everything
	if( !ready ) {

		for( int i=0; i<mux->N; i++ ) {
			
			Waitqueue* waitq = (channelRead == mux->ops[i]
			                    ? &mux->channels[i]->readq
			                    : &mux->channels[i]->writeq);
			sleep_waitqueue_Job( NULL, waitq, job );

		}
		// For returning to caller
		ready = channelBlocked;

	}

	// Unlock all of them (in reverse order)
	for( int i=0; i<mux->N; i++ )
		unlock_SPINLOCK( &mux->channels[ mux->N - i - 1 ]->lock );
	
	unlock_SPINLOCK( &mux->lock );

	return ready;
}

Chanmux*       new_Chanmux( int      n, 
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

	mux->R        = R;
	
	mux->N        = n;
	
	mux->ops      = ralloc( R, n * sizeof(muxOp_e) );
	mux->channels = ralloc( R, n * sizeof(Channel*) );
	mux->sizes    = ralloc( R, n * sizeof(uint16) );
	mux->ptrs     = ralloc( R, n * sizeof(pointer));
	
	mux->stati    = ralloc( R, n * sizeof(int) );

	memcpy( mux->ops, ops, n * sizeof(int) );
	memcpy( mux->channels, channels, n * sizeof(Channel*) );
	memcpy( mux->sizes, sizes, n * sizeof(uint16) );
	memcpy( mux->ptrs, ptrs, n * sizeof(pointer) );
	memset( mux->stati, 0, n * sizeof(int) );

	return mux;

}

void       destroy_Chanmux( Chanmux* mux ) {

	destroy_SPINLOCK( &mux->lock );
	rfree(mux->R);

}

int          first_Chanmux( const Chanmux* mux ) {

	for( int i=0; i<mux->N; i++ ) {
		if( mux->stati[i] > 0 )
			return i;
	}

	return channelEof;

}

int           next_Chanmux( const Chanmux* mux, int from ) {

	for( int i=from+1; i<mux->N; i++ ) {
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

#include "core.types.h"
#include "job.control.h"
#include "job.core.h"

typedef struct {

	uint16 N;
	uint64 fib_N;

} Result;

declare_job( int, producer, Channel* out );
declare_job( int, consumer, Channel* in );
declare_job( int, repeater, Channel* src; Channel* dest );

declare_job( unsigned long long, fibonacci,    unsigned long long n );
declare_job( int,                fib_producer, Channel* out );
declare_job( int,                fib_consumer, Chanmux* mux );

//

define_job( unsigned long long, fibonacci,

            unsigned long long n_1;
            unsigned long long n_2;

            Handle job_n_1;
            Handle job_n_2 )
{
	begin_job;

	if( 0 == arg( n ) ) {

		exit_job( 0 );

	} else if( 1 == arg( n ) ) {

		exit_job( 1 );

	}

	call_job( local(job_n_1), arg(n) - 1, cpuBound, &local(n_1), fibonacci, arg(n) - 1 );
	call_job( local(job_n_2), arg(n) - 2, cpuBound, &local(n_2), fibonacci, arg(n) - 2 );
		
	exit_job( local(n_1) + local(n_2) );

	end_job;
}

define_job( int, fib_producer,

            uint16 i;

            Handle fib_i;
            Result fib ) {

	begin_job;

	while( 1 ) {

		call_job( local(fib_i), 
		          (uint32)local(i), 
		          cpuBound, 
		          &local(fib).fib_N, 
		          fibonacci, 
		          local(i) );
		local(fib).N = local(i);
		writech( arg(out), local(fib) );

		++local(i);

	}
	flushch( arg(out) );

	end_job;
}

define_job( int, fib_consumer,
            
            uint16   i;
            Result   fib ) {

	begin_job;

	while( 1 ) {

		muxch( arg(mux), n ) {

			Result* fib = muxchi( arg(mux), Result, n );
			printf("%d: fib(%d) = %llu\n", n, fib->N, fib->fib_N);
			
		};
/*
		muxch( arg(mux) );
		for( int i=first_Chanmux(arg(mux));
		     i >= 0;
		     i = next_Chanmux(arg(mux), i) ) {

			local(fib) = *(Result*)data_Chanmux( arg(mux),i );

			printf("%d: fib(%d) = %llu\n", i, local(fib).N, local(fib).fib_N);

		}
*/
		++local(i);
		
	}

	end_job;
}

int main(int argc, char* argv[] ) {

	region_p R = region("job.channel.TEST");

	if( argc < 3 ) {
		fprintf(stderr, "usage: %s <n_workers> <n_producers>\n", argv[0]);
		return 1;
	}

	int n_threads = (int)strtol( argv[1], NULL, 10 );
	int n_producers = (int)strtol( argv[2], NULL, 10 );

	init_Jobs( n_threads );

	// Make the channel size not a multiple of sizeof(int)
	// this ensures we exercise more code paths.
	muxOp_e*  ops = ralloc( R, n_producers * sizeof(muxOp_e) );
	Channel** chs = ralloc( R, n_producers * sizeof(Channel*) );
	uint16* sizes = ralloc( R, n_producers * sizeof(Result) );
	pointer* ptrs = ralloc( R, n_producers * sizeof(pointer*) );
	typeof_Job_params(fib_producer)* p_params = ralloc( R,
	                                                    n_producers *
	                                                    sizeof(typeof_Job_params(fib_producer)) );
	int*    p_ret = ralloc( R, n_producers * sizeof(int) );

	for( int i=0; i<n_producers; i++ ) {

		chs[i]   = new_Channel( sizeof(Result), 1 );
		ops[i]   = channelRead;
		sizes[i] = sizeof(Result);
		ptrs[i]  = ralloc( R, sizeof(Result) );
		p_params[i] = (typeof_Job_params(fib_producer)){ chs[i] };

		submit_Job( 0, ioBound, &p_ret[i], (jobfunc_f)fib_producer, &p_params[i] );
		
	}

	Chanmux* mux = new_Chanmux(n_producers,
	                           ops,
	                           chs,
	                           sizes,
	                           ptrs );
	
	int c_ret;
	typeof_Job_params(fib_consumer) c_params = { mux };
	Handle cons = submit_Job( 0, ioBound, &c_ret, (jobfunc_f)fib_consumer, &c_params);

	mutex_t mutex; init_MUTEX(&mutex);
	condition_t cond; init_CONDITION(&cond);

	fprintf(stdout, "submitted %d producers; consumer: %p\n",
	        n_producers, deref_Handle(Job*,cons));

	lock_MUTEX(&mutex);
	while( join_deadline_Job( 0, &mutex, &cond ) < 0 );
	unlock_MUTEX(&mutex);

	for( int i=0; i<n_producers; i++ )
		destroy_Channel(chs[i]);
	destroy_Chanmux( mux );
	rcollect( R );

	shutdown_Jobs();

	return 0;
}

#endif
