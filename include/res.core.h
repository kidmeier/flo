#ifndef __res_core_h__
#define __res_core_h__

#include "core.types.h"
#include "time.core.h"

typedef struct resource_s  resource_t;
typedef struct resource_s* resource_p;
struct resource_s {

	char*   name;

	size_t  size;
	pointer data;

	unsigned int   refcount;
	msec_t         timestamp;
	msec_t         expiry;

	resource_p prev, next;

};

typedef resource_p (*load_resource_f)( int size, const void* buf );

#define resNeverExpire 0

void       register_loader_RES( const char* ext, load_resource_f loadfunc );
void       add_path_RES( const char* protocol, const char* prefix );
resource_p create_raw_RES( int size, void* data, msec_t expiry );
resource_p load_RES( const char* url, int size_hint );
resource_p get_RES( const char* url );
void       put_RES( const resource_p res );

resource_p load_resource_TXT( int size, const void *buf );

#define resource( typ, url )	\
	(typ *)get_RES( url )->data

#endif
