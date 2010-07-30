#include "job.channel.h"
#include "job.queue.h"
#include "core.alloc.h"

struct job_channel_s {

	spinlock_t  lock;
	job_queue_p waitqueue;

	uint16 readp;
	uint16 writep;

	uint16 buf_size;
	byte   buf[];

};

job_channel_p new_CHAN( uint16 size, uint16 count ) {

	job_channel_p chan = (job_channel_p)alloc( NULL, sizeof(job_channel_t) + size*count );
	
	if( init_SPINLOCK(&chan->lock) ) {
		delete(chan);
		return NULL;
	}

	chan->waitqueue = NULL;
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
	if( (writep - chan->buf_size + size) > readp ) {

		sleep_waitqueue_JOB( NULL, &chan->waitqueue, job );
		unlock_SPINLOCK( &chan->lock );

		return channelBlocked;

	}
	
	// Copy
	if( writep + size >= chan->buf_size ) {

		memcpy( &chan->buf[writep], data, writep + size - chan->buf_size );
		memcpy( &chan->buf[0], data, chan->buf_size - writep );

	} else
		memcpy( &chan->buf[writep], data, size );

	chan->writep = (writep + size) % chan->buf_size;
	unlock_SPINLOCK( &chan->lock );

	return size;
}

int           read_CHAN( job_queue_p job, job_channel_p chan, uint16 size, pointer dest ) {

	lock_SPINLOCK( &chan->lock );
	
	int writep = chan->writep;
	int readp = chan->readp;
	int endp = (readp + size) % chan->buf_size;

	// Can't crossover the read pointer
	if( endp > writep ) {
		
		sleep_waitqueue_JOB( NULL, &chan->waitqueue, job );
		unlock_SPINLOCK( &chan->lock );

		return channelBlocked;

	}
	
	// Copy
	if( readp + size >= chan->buf_size ) {

		uint16 first_chunk_length  = readp + size - chan->buf_size;
		uint16 second_chunk_length = chan->buf_size - readp;

		memcpy( dest,                      &chan->buf[readp], first_chunk_length );
		memcpy( dest + first_chunk_length, &chan->buf[0],     second_chunk_length );

	} else
		memcpy( dest, &chan->buf[readp], size );

	chan->readp = endp;
	unlock_SPINLOCK( &chan->lock );

	return size;

}
