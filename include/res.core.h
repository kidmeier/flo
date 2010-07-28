#ifndef __res_core_h__
#define __res_core_h__

#include "core.types.h"

struct resource_s;
typedef struct resource_s resource_t;
typedef resource_t* resource_p;
struct resource_s {

	char*   name;

	size_t  size;
	pointer data;

	unsigned int   refcount;
	struct timeval timestamp;
	struct timeval expiry;

	resource_p prev, next;

};

typedef resource_p (*load_resource_f)( int size, const void* buf );

void       register_loader_RES( const char* ext, load_resource_f loadfunc );
void       add_path_RES( const char* protocol, const char* prefix );
resource_p create_raw_RES( int size, void* data, int64 expiry );
resource_p load_RES( const char* url, int size_hint );
resource_p get_RES( const char* url );
void       put_RES( const resource_p res );

#define resource( typ, url )	\
	(typ *)get_RES( url )->data

#endif
