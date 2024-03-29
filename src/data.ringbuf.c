#include <stdlib.h>
#include <string.h>

#include "data.ringbuf.h"

struct ringbuf_s {
	
	uint16 readp;
	uint16 writep;

	uint32 bytes_read;
	uint32 bytes_written;

	uint16 buf_size;
	byte   buf[];

};

ringbuf_p new_RINGBUF( uint16 size, uint16 count ) {

	ringbuf_p ring = (ringbuf_p)malloc( sizeof(ringbuf_t) + size*count );
	
	ring->readp = 0;
	ring->writep = 0;

	ring->bytes_read = 0;
	ring->bytes_written = 0;

	ring->buf_size = size*count;
	memset( &ring->buf[0], 0, ring->buf_size );

	return ring;

}

void  destroy_RINGBUF( ringbuf_p ring ) {

	free(ring);

}

int remaining_RINGBUF( ringbuf_p ring ) {

	return ring->buf_size - (ring->bytes_written - ring->bytes_read);

}

// Writes `size` bytes to `buf` from `data`. Returns `size` on success,
// or the negated maximum number of bytes that can be written (e.g. -4 means 
// a maximum of 4 bytes can be written until some bytes are read)
int     write_RINGBUF( ringbuf_p ring, uint16 size, const pointer data ) {

	int writep = ring->writep;

	// Can't write more than is remaining
	if( remaining_RINGBUF( ring ) < size )
		return -(size - remaining_RINGBUF(ring));

	// copy with-wrap
	if( writep + size > ring->buf_size ) {

		uint16 first_chunk_length  = ring->buf_size - writep;
		uint16 second_chunk_length = size - first_chunk_length;

		memcpy( &ring->buf[writep], data,                      first_chunk_length );
		memcpy( &ring->buf[0],      data + first_chunk_length, second_chunk_length );

	} else
		memcpy( &ring->buf[writep], data, size );

	ring->writep = (writep + size) % ring->buf_size;
	ring->bytes_written += size;

	return size;

}

int available_RINGBUF( ringbuf_p ring ) {

	return ring->bytes_written - ring->bytes_read;

}

// Reads `size` bytes from `buf` to `data`. Returns `size` on success,
// or the negated maximum number of bytes that can be read (e.g. -4 means 
// a maximum of 4 bytes can be read until some more bytes are written)
int      read_RINGBUF( ringbuf_p ring, uint16 size, pointer dest ) {

	int readp = ring->readp;

	// Can't read more than is available
	if( available_RINGBUF(ring) < size )
		return -(size - available_RINGBUF(ring));

	// copy with-wrap
	if( readp + size > ring->buf_size ) {

		uint16 first_chunk_length  = ring->buf_size - readp;
		uint16 second_chunk_length = size - first_chunk_length;

		memcpy( dest,                      &ring->buf[readp], first_chunk_length );
		memcpy( dest + first_chunk_length, &ring->buf[0],     second_chunk_length );

	} else
		memcpy( dest, &ring->buf[readp], size );

	ring->readp = (readp + size) % ring->buf_size;
	ring->bytes_read += size;

	return size;

}

#ifdef __data_ringbuf_TEST__

#include <stdio.h>
#include <stdlib.h>

void produce( ringbuf_p ring ) {

	int i = rand() % 1000;
	while( write_RINGBUF(ring, sizeof(i), &i) > 0 ) {
		printf("Wrote: % 6d\n", i);
		i = rand() % 1000;
	}

}

void consume( ringbuf_p ring ) {

	int i;
	while( read_RINGBUF(ring, sizeof(i), &i) > 0 ) {
		printf("Read:  % 6d\n", i);
	}

}

int main( int argc, char* argv[] ) {

	// Use prime # to ensure we exercise all code paths
	ringbuf_p ring = new_RINGBUF( sizeof(char), 13 );
	
	for( int i=0; i<100; i++ ) {
		produce(ring);
		consume(ring);
	}

	destroy_RINGBUF(ring);
	return 0;

}

#endif
