#include "job.channel.h"
#include "job.queue.h"
#include "core.alloc.h"

struct job_channel_s {

	spinlock_t  lock;

	job_queue_p readq;
	job_queue_p writeq;

	uint16 readp;
	uint16 writep;

	uint32 bytes_read;
	uint32 bytes_written;

	uint16 buf_size;
	byte   buf[];

};

job_channel_p new_CHAN( uint16 size, uint16 count ) {

	job_channel_p chan = (job_channel_p)alloc( NULL, sizeof(job_channel_t) + size*count );
	
	if( init_SPINLOCK(&chan->lock) ) {
		delete(chan);
		return NULL;
	}

	chan->readq = NULL;
	chan->writeq = NULL;

	chan->readp = 0;
	chan->writep = 0;

	chan->buf_size = size*count;
	memset( &chan->buf[0], 0, chan->buf_size );

	return chan;

}

void          destroy_CHAN( job_channel_p chan ) {

	// TODO: What action to take if there are blocked jobs on chan?
	destroy_SPINLOCK( &chan->lock );
	delete( chan );

}

int           write_CHAN( job_queue_p job, job_channel_p chan, uint16 size, pointer data ) {

	lock_SPINLOCK( &chan->lock );

	int writep = chan->writep;
	int readp = chan->readp;

	// Can't crossover to the read ptr
	if( chan->bytes_written + size > chan->bytes_read + chan->buf_size ) {

		sleep_waitqueue_JOB( NULL, &chan->writeq, job );
		unlock_SPINLOCK( &chan->lock );

		return channelBlocked;

	}
	
	// copy with-wrap
	if( writep + size > chan->buf_size ) {

		uint16 first_chunk_length  = chan->buf_size - writep;
		uint16 second_chunk_length = size - first_chunk_length;

		memcpy( &chan->buf[writep], data,                      first_chunk_length );
		memcpy( &chan->buf[0],      data + first_chunk_length, second_chunk_length );

	} else
		memcpy( &chan->buf[writep], data, size );

	chan->writep = (writep + size) % chan->buf_size;
	chan->bytes_written += size;

	wakeup_waitqueue_JOB( NULL, &chan->readq );

	unlock_SPINLOCK( &chan->lock );

	return size;
}

int           read_CHAN( job_queue_p job, job_channel_p chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );
	
	int writep = chan->writep;
	int readp = chan->readp;

	// Can't crossover the read pointer
	if( chan->bytes_read + size > chan->bytes_written ) {
		
		sleep_waitqueue_JOB( NULL, &chan->readq, job );
		unlock_SPINLOCK( &chan->lock );

		return channelBlocked;

	}

	// copy with-wrap
	if( readp + size > chan->buf_size ) {

		uint16 first_chunk_length  = chan->buf_size - readp;
		uint16 second_chunk_length = size - first_chunk_length;

		memcpy( dest,                      &chan->buf[readp], first_chunk_length );
		memcpy( dest + first_chunk_length, &chan->buf[0],     second_chunk_length );

	} else
		memcpy( dest, &chan->buf[readp], size );

	chan->readp = (readp + size) % chan->buf_size;
	chan->bytes_read += size;

	wakeup_waitqueue_JOB( NULL, &chan->writeq );

	unlock_SPINLOCK( &chan->lock );

	return size;
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

	return 0;
}

#endif
