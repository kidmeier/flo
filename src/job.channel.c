#include <assert.h>

#include "data.ringbuf.h"
#include "job.channel.h"
#include "job.queue.h"
#include "core.alloc.h"

struct job_channel_s {

	spinlock_t  lock;

	job_queue_p readq;
	job_queue_p writeq;

	job_chanalt_p alt_read;
	job_chanalt_p alt_write;

	ringbuf_p   ring;

};

struct job_chanalt_s {

	spinlock_t   lock;

	job_queue_p waitq;

	int            N;

	int*           alts;
	job_channel_p* channels;
	uint16*        sizes;
	pointer*       ptrs;

	int*           stati;

};

// Primitive ops

// Try to write `size` bytes to channel from 'data`; returns channelBlocked if
// not enough bytes left in buffer; returns `size` on success
//
// Assume `chan` is appropriately locked by caller
static int try_write( job_channel_p chan, uint16 size, pointer data ) {

	int ret = write_RINGBUF(chan->ring, size, data);
	if( ret < 0 )
		return channelBlocked;
	
	wakeup_waitqueue_JOB( NULL, &chan->readq );
	if( chan->alt_read ) 
		wakeup_waitqueue_JOB( &chan->alt_read->lock, &chan->alt_read->waitq );

	return ret;

}

// Try to read `size` bytes from channel into 'dest`; returns channelBlocked if
// not enough bytes are available for reading; returns `size` on success
//
// Assume `chan` is appropriately locked by caller
static int try_read( job_channel_p chan, uint16 size, pointer dest ) {

	int ret = read_RINGBUF( chan->ring, size, dest );
	if( ret < 0 )
		return channelBlocked;	

	wakeup_waitqueue_JOB( NULL, &chan->writeq );
	if( chan->alt_write ) 
		wakeup_waitqueue_JOB( &chan->alt_write->lock, &chan->alt_write->waitq );

	return ret;

}

// Public API /////////////////////////////////////////////////////////////////

job_channel_p new_CHAN( uint16 size, uint16 count ) {

	job_channel_p chan = new( NULL, job_channel_t );
	
	if( init_SPINLOCK(&chan->lock) ) {
		delete(chan);
		return NULL;
	}

	chan->readq = NULL;
	chan->writeq = NULL;

	chan->alt_read = NULL;
	chan->alt_write = NULL;

	chan->ring = new_RINGBUF( size, count );
	if( !chan->ring ) {
		destroy_SPINLOCK(&chan->lock);
		delete(chan);

		return NULL;
	}
	adopt( chan, chan->ring );

	return chan;

}

job_chanalt_p new_CHAN_alt( int n, 
                            int alts[], 
                            job_channel_p channels[], 
                            uint16 sizes[], 
                            pointer ptrs[] ) {
	
	job_chanalt_p alt = new( NULL, job_chanalt_t );
	
	init_SPINLOCK( &alt->lock );
	
	alt->waitq = NULL;
	
	alt->N = n;
	
	alt->alts = new_array( alt, int, n );
	alt->channels = new_array( alt, job_channel_p, n );
	alt->sizes = new_array( alt, uint16, n );
	alt->ptrs = new_array( alt, pointer, n );
	
	alt->stati = new_array( alt, int, n );

#ifdef DEBUG
	for( int i=0; i<n; i++ ) {
		assert( NULL == channels[i]->alt_read 
		        && NULL == channels[i]->alt_write );
	}
#endif
	
	memcpy( alt->alts, alts, n * sizeof(int) );
	memcpy( alt->channels, channels, n * sizeof(job_channel_p) );
	memcpy( alt->sizes, sizes, n * sizeof(uint16) );
	memcpy( alt->ptrs, ptrs, n * sizeof(pointer) );
	memcpy( alt->stati, 0, n * sizeof(int) );

	return alt;

}

void          destroy_CHAN( job_channel_p chan ) {

	// TODO: What action to take if there are blocked jobs on chan?
	destroy_SPINLOCK( &chan->lock );
	delete( chan );

}

void          destroy_CHAN_alt( job_chanalt_p chanalt ) {

	destroy_SPINLOCK( &chanalt->lock );
	delete(chanalt);

}

int           write_CHAN( job_queue_p job, job_channel_p chan, uint16 size, pointer data ) {

	lock_SPINLOCK( &chan->lock );

	int ret = try_write( chan, size, data );
	if( channelBlocked == ret ) {
		sleep_waitqueue_JOB( NULL, &chan->writeq, job );
	}

	unlock_SPINLOCK( &chan->lock );

	return ret;
}

int try_write_CHAN( job_channel_p chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );
	int ret = try_write( chan, size, dest );
	unlock_SPINLOCK( &chan->lock );
	
	return ret;
	
}

int           read_CHAN( job_queue_p job, job_channel_p chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );

	int ret = try_read( chan, size, dest );
	if( channelBlocked == ret ) {
		sleep_waitqueue_JOB( NULL, &chan->readq, job );
	}

	unlock_SPINLOCK( &chan->lock );

	return ret;
}

int try_read_CHAN( job_channel_p chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );
	int ret = try_read( chan, size, dest );
	unlock_SPINLOCK( &chan->lock );
	
	return ret;
	
}

int alt_CHAN( job_queue_p job, job_chanalt_p chanalt ) {

	int ready = 0;

	lock_SPINLOCK( &chanalt->lock );
	for( int i=0; i<chanalt->N; i++ ) {
		
		int           alt = chanalt->alts[i];
		job_channel_p chan = chanalt->channels[i];
		uint16        size = chanalt->sizes[i];
		pointer       ptr = chanalt->ptrs[i];
		int*          status = &chanalt->stati[i];
		
		job_chanalt_p* chan_alt;
		job_chanalt_p* chan_alt_other;
		int (*try_alt)( job_channel_p, uint16, pointer );

		lock_SPINLOCK( &chan->lock );
		switch( alt ) {
		case channelRead:
			chan_alt = &chan->alt_read;
			chan_alt_other = &chan->alt_write;
			try_alt = try_read_CHAN;
			break;
		case channelWrite:
			chan_alt = &chan->alt_write;
			chan_alt_other = &chan->alt_read;
			try_alt = try_write_CHAN;
			break;
		}
		
		// Can only be involved in one alt at a time; either us or nobody
		assert( NULL == *(chan_alt) || chanalt == *(chan_alt) );
		// We can't wait on a read and a write in the same alt
		assert( NULL == *(chan_alt_other) || chanalt != *(chan_alt_other) );

		*(status) = (try_alt)( chan, size, ptr );

		// Success; clear the chanalt from the channel and inc our `ready` counter
		if( channelBlocked != *(status) ) {

			*(chan_alt) = NULL;
			ready++;
			
		} else 
			// Would block; mark ourself as waiting in the channel's appropriate `alt`
			*(chan_alt) = chanalt;

		unlock_SPINLOCK( &chan->lock );
		
	}
	
	// If nothing is ready tell the caller to block
	if( !ready ) {
		sleep_waitqueue_JOB( NULL, &chanalt->waitq, job );
		ready = channelBlocked;
	}
	unlock_SPINLOCK( &chanalt->lock );

	return ready;
}

int           first_CHAN_alt( const job_chanalt_p chanalt ) {

	for( int i=0; i<chanalt->N; i++ ) {
		if( chanalt->stati[i] > 0 )
			return i;
	}

	return channelEof;

}

int           next_CHAN_alt( const job_chanalt_p chanalt, int from ) {

	for( int i=from; i<chanalt->N; i++ ) {
		if( chanalt->stati[i] > 0 )
		    return i;
	}

	return channelEof;

}

pointer       data_CHAN_alt( const job_chanalt_p chanalt, int at ) {

	assert( at >= 0 && at < chanalt->N );
	return chanalt->ptrs[at];

}

uint16        size_CHAN_alt( job_chanalt_p chanalt, int at ) {

	assert( at >= 0 && at < chanalt->N );
	return chanalt->sizes[at];

}

#ifdef __job_channel_TEST__

#include <stdio.h>
#include "job.control.h"
#include "job.core.h"

declare_job( int, producer, job_channel_p out );
declare_job( int, consumer, job_channel_p in );

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

	init_JOBS();

	// Make the channel size not a multiple of sizeof(int)
	// this ensures we exercise more code paths.
	job_channel_p chan = new_CHAN( sizeof(char), 13 );

	int c_ret;
	consumer_job_params_t c_params = { chan };

	int p_ret;
	producer_job_params_t p_params = { chan };

	jobid cons = submit_JOB( nullJob, 0, ioBound, &c_ret, (jobfunc_f)consumer, &c_params);
	jobid prod = submit_JOB( nullJob, 0, ioBound, &p_ret, (jobfunc_f)producer, &p_params);

	mutex_t mutex; init_MUTEX(&mutex);
	condition_t cond; init_CONDITION(&cond);

	fprintf(stdout, "jobs submitted; watch the magic\n");

	lock_MUTEX(&mutex);
	while( join_deadline_JOB( 0, &mutex, &cond ) < 0 );
	unlock_MUTEX(&mutex);

	destroy_CHAN(chan);

	return 0;
}

#endif
