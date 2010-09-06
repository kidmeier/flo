#include <assert.h>
#include <string.h>

#include "core.log.h"
#include "data.list.h"
#include "mm.region.h"
#include "sync.spinlock.h"

static const int allocAlign  = 16;

struct page_s {

	uint16         pp;
	slist_mixin( struct page_s );

};

struct region_s {

	char*    name;
	zone_p   zone;

	struct page_s* first;
	struct page_s* last;

};

static zone_p default_zone = NULL;

// Internal APIs //////////////////////////////////////////////////////////////

#define round_up_nearest( alignment, sz )	  \
	( (sz) <= (alignment) ) ? (alignment) : (sz) + (alignment) - ((sz)&((alignment)-1))

static struct page_s* alloc_page( zone_p Z ) {

	struct page_s* page = NULL;

	trace("alloc_page(%s)", Z->name);

	lock_SPINLOCK( Z->freelist_lock );
	if( NULL == Z->freelist ) {		
		unlock_SPINLOCK( Z->freelist_lock );

		trace0("alloc_page: no free pages, allocate new one");

		page = (struct page_s*)zalloc( Z, Z->page_sz );
		if( !page )
			return NULL;
		
		page->pp = round_up_nearest( allocAlign, sizeof(struct page_s) );
		slist_next(page) = NULL;
		
	} else {
		
		slist_pop_front( Z->freelist, page );
		unlock_SPINLOCK( Z->freelist_lock );

		page->pp = round_up_nearest( allocAlign, sizeof(struct page_s) );

	}

	trace("alloc_page: return 0x%x", (uint)page);
	return page;

}

static void free_page( zone_p Z, struct page_s* page ) {

	trace("free_page(%s, 0x%x)", Z->name, (uint)page);

	lock_SPINLOCK( Z->freelist_lock );
	slist_push_front( Z->freelist, page );
	unlock_SPINLOCK( Z->freelist_lock );

}

// Public API /////////////////////////////////////////////////////////////////

region_p region( zone_p Z, const char* name ) {

	trace("region(%s, %s)", Z->name, name);
	
	region_p R = (region_p)zalloc( default_zone, sizeof(region_t) );
	if( !R )
		return NULL;

	R->name = zalloc( default_zone, strlen(name)+1 );
	strcpy( R->name, name );
	R->zone = Z;

	R->first = R->last = NULL;//alloc_page( Z );

	return R;

}

pointer  ralloc( region_p R, uint16 sz ) {
	
	uint16 aligned_sz = round_up_nearest( allocAlign, sz );
	assert( aligned_sz <= R->zone->page_sz );

	// Allocate a new page if needed
	if( !R->last ) {

		R->first = R->last = alloc_page( R->zone );

	} else if( R->last->pp + aligned_sz >= R->zone->page_sz ) {

		struct page_s* next = alloc_page( R->zone );
		R->last->next = next;
		R->last = next;

	}

	// Do the allocation
	pointer m = (pointer)R->last + R->last->pp;
	R->last->pp += aligned_sz;

	return m;

}

void     rfree( region_p R ) {

	trace("rfree(%s)", R->name);

	// Free region pages
	struct page_s* page = R->first;
	while( page ) {
		struct page_s* next = slist_next(page);
		free_page( R->zone, page );
		page = next;
	}

	R->first = R->last = NULL;

}

int      region_MM_init( zone_p zone ) {

	assert( NULL != zone );

	trace( "region_MM_init(%s)", zone->name );

	default_zone = zone;
	return 0;
	
}

void     region_MM_shutdown( void ) {

	// Nop for now

}

#ifdef __mm_region_TEST__

#include <stdio.h>

#include "mm.heap.h"

int main( int argc, char* argv[] ) {

	region_MM_init( ZONE_heap );

	const int REPS = 1024;
	const char* TEST = "TEST";
	region_p R = region( ZONE_heap, TEST );
	
	int length = strlen(TEST);
	for( int i=0; i<REPS; i++ ) {
		pointer  p = ralloc( R, length+1 );
	}

	info0( "Freeing region" );
	rfree( R );
	
	info0( "Running test again; pages should be allocated in reverse order from above." );
	for( int i=0; i<REPS; i++ ) {
		pointer  p = ralloc( R, length+1 );
	}

	// Now try allocating more than a page
	uint size = R->zone->page_sz + 1;
	pointer chunk = ralloc( R, size );
	info( "Allocated chunk: 0x%x, size=%d", (uint)chunk, size );

	rfree( R );
}

#endif
