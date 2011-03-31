#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "core.log.h"
#include "data.list.mixin.h"
#include "mm.heap.h"
#include "mm.region.h"
#include "sync.atomic.h"
#include "sync.spinlock.h"

static const int pageSize    = 16 * 1024;
static const int allocAlign  = 64;

struct page{

	uint  pp;
	slist_mixin( struct page );

};

struct region_s {

	char*    name;

	struct page* first;
	struct page* last;

};

static threadlocal struct page* freelist = NULL;

// Internal APIs //////////////////////////////////////////////////////////////

#define align( alignment, sz )	  \
	( (sz) <= (alignment) ) ? (alignment) : (sz) + (alignment) - ((sz)&((alignment)-1))

// Ref: http://graphics.stanford.edu/~seander/bithacks.html
static inline 
unsigned nearest_pow2( unsigned x ) {

    --x;  

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    
    return ++x;

}

static struct page* alloc_page( void ) {

	struct page* pg = NULL;

	trace0("alloc_page()");

	if( NULL == freelist ) {		

		trace0("alloc_page: no free pages, allocate new one");

		pg = (struct page*)malloc( pageSize  );
		if( !pg )
			return NULL;
		
		pg->pp = align( allocAlign, sizeof(struct page) );
		slist_next(pg) = NULL;
		
	} else {
		
		slist_pop_front( freelist, pg );
		pg->pp = align( allocAlign, sizeof(struct page) );

	}

	trace("alloc_page: return 0x%x", (uint)pg);
	return pg;

}

static void free_page( struct page* pg ) {

	trace("free_page(0x%x)", (uint)pg);

	slist_push_front( freelist, pg );

}

// Public API /////////////////////////////////////////////////////////////////

region_p region( const char* name ) {

	trace("region(%s)", name);
	
	region_p R = (region_p)malloc( sizeof(region_t) + strlen(name) + 1 );
	if( !R )
		return NULL;
	R->name = (pointer)R + sizeof(region_t);

	strcpy( R->name, name );
	R->first = R->last = NULL;

	return R;

}

pointer  ralloc( region_p R, uint16 sz ) {
	
	uint16 aligned_sz = align( allocAlign, sz );
	assert( aligned_sz <= pageSize );

	// Allocate a new page if needed
	if( !R->last ) {

		R->first = R->last = alloc_page( );

	} else if( R->last->pp + aligned_sz > pageSize ) {

		struct page* next = alloc_page( );
		R->last->next = next;
		R->last = next;

	}

	// Do the allocation
	pointer       m = (pointer)R->last + R->last->pp;
	R->last->pp    += aligned_sz;

	return m;

}

pointer  rallocpg( region_p R ) {

	return alloc_page( );

}

void     rfreepg( region_p R, const pointer pg ) {

	struct page* page = (struct page*)pg;

	page->next = NULL;
	free_page( page );

}

void     rcollect( region_p R ) {

	trace("rcollect(%s)", R->name);

	// Free region pages
	struct page* pg = R->first;
	while( pg ) {
		struct page* next = slist_next(pg);
		free_page( pg );
		pg = next;
	}

	R->first = R->last = NULL;

}

void     rfree( region_p R ) {

	rcollect( R );
	free( R );

}

#ifdef __mm_region_TEST__

#include <stdio.h>

int main( int argc, char* argv[] ) {

	const int REPS = 1024;
	const char* TEST = "TEST";
	region_p R = region( TEST );
	
	int length = strlen(TEST);
	for( int i=0; i<REPS; i++ ) {
		pointer  p = ralloc( R, length+1 );
		(void)p;
	}

	info0( "Freeing region" );
	rfree( R );
	
	info0( "Running test again; pages should be allocated in reverse order from above." );
	for( int i=0; i<REPS; i++ ) {
		pointer  p = ralloc( R, length+1 );
		(void)p;
	}

	// Now try allocating more than a page
	uint size = pageSize + 1;
	pointer chunk = ralloc( R, size );
	info( "Allocated chunk: %p, size=%d", chunk, size );

	rfree( R );

}

#endif
