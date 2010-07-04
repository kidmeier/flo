#ifndef __res_core_h__
#define __res_core_h__

#include "core.types.h"

struct resource_s;
typedef struct resource_s resource_t;
typedef resource_t* resource_p;

typedef resource_p (*load_resource_f)( int size, const void* buf );

void       register_loader_RES( const char* ext, load_resource_f loadfunc );
resource_p create_raw_RES( int size, void* data, int64 expiry );
resource_p load_RES( const char* url, int size_hint );
resource_p get_RES( const char* url );
void       put_RES( resource_p res );

#endif
