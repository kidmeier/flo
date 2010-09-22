#ifndef __mm_zone_h__
#define __mm_zone_h__

#include <string.h>

#include "core.types.h"
#include "sync.spinlock.h"

// Forward
struct page_s;

typedef struct zone_s  zone_t;
typedef struct zone_s* zone_p;

struct zone_s {

	const char* name;
	
	// Regional allocator info
	struct page_s* freelist;
	spinlock_t*    freelist_lock;

	uint16         page_sz;

	pointer (*alloc)( zone_p Z, int sz );
	pointer (*realloc)( zone_p Z, pointer p, int sz );
	void    (*free )( zone_p Z, pointer p );

};	

static inline
pointer  zalloc( zone_p Z, int sz ) {
	return Z->alloc( Z, sz );
}

static inline
void     zfree( zone_p Z, pointer p ) {
	Z->free( Z, p );
}

static inline
pointer zrealloc( zone_p Z, pointer p, int old_sz, int sz ) {

	// Optional operation
	if( Z->realloc )
		return Z->realloc( Z, p, sz );
	else {

		// Fallback; this is why we need to know the previous size
		pointer newp = Z->alloc( Z, sz );
		if( !newp )
			return NULL;

		memcpy( newp, p, (old_sz < sz ? old_sz : sz) );
		zfree( Z, p );

		return newp;

	}
		
}	

#endif
