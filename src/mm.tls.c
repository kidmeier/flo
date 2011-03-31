#include <stdio.h>
#include <stdlib.h>

#include "mm.tls.h"
#include "sync.thread.h"

// Default to 512 kb of stack space per thread
#ifndef tls_MM_size
#define tls_MM_size 512 * 1024
#endif

static threadlocal pointer base  = NULL;
static threadlocal pointer sp    = NULL;
static threadlocal pointer limit = NULL;

static void leakcheck( void ) {

	if( base != sp ) {

		fprintf(stderr, "LEAKCHECK: thread %x: %s:%d: Garbage left on stack:\n",
		        (unsigned)self_THREAD(), __FILE__, __LINE__ );
		fprintf(stderr, "\tbase=%p\tsp=%p\tsize=%td\n", 
		        base, sp, sp - base );

	}

}

static pointer SP( void ) {

	if( !base ) {
		base  = malloc( tls_MM_size );
		sp    = base;
		limit = base + tls_MM_size;

		atexit( leakcheck );
	}
	return sp;

}		

pointer pushtls( uint size ) {

	pointer ptr = SP();
	if( ptr + size > limit ) {
		fprintf(stderr, "thread 0x%x: %s:%d: Stack overflow\n", 
		        (unsigned)self_THREAD(), __FILE__, __LINE__ );
		abort();
	}

	sp = SP() + size;
	return ptr;

}

void    poptls( uint size ) {

	if( size > sp - base ) {
		
		fprintf(stderr, "thread 0x%x: %s:%d: Stack underflow\n",
		        (unsigned)self_THREAD(), __FILE__, __LINE__ );
		abort();
	}		
	sp = SP() - size;

}
