#include "core.log.h"
#include "mm.zone.h"

pointer  zalloc( const zone_p zone, int sz ) {

	return zone->alloc( zone, sz );

}

void     zfree( const zone_p zone, pointer p ) {

	zone->free( zone, p );

}
