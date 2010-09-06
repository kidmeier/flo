#include <assert.h>
#include <stdlib.h>
//
#include "mm.heap.h"
#include "sync.once.h"

#define heapPageSize 4096
static spinlock_t lock;

static void    heap_init( void ) {
	init_SPINLOCK( &lock );
}

static pointer heap_alloc( zone_p zone, int sz ) {

	assert( ZONE_heap == zone );
	once( heap_init );

	return malloc(sz);

}

static void    heap_free( zone_p zone, pointer p ) {

	assert( ZONE_heap == zone );
	free( p );

}

static zone_t heap = {

	.name  = "HEAP",

	.freelist      = NULL,
	.freelist_lock = &lock,
	.page_sz       = heapPageSize,

	.alloc = heap_alloc,
	.free  = heap_free

};
zone_p ZONE_heap = &heap;
