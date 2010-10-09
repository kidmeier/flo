#ifndef __data_map_h__
#define __data_map_h__

#include "core.types.h"
#include "mm.zone.h" 

typedef struct Map Map;

// Instantiation
Map*         new_Map( zone_p Z, int initial_capacity );
void      delete_Map( Map* map );

// Functions
uint        size_Map( const Map* map );
float       load_Map( const Map* map );
pointer   lookup_Map( const Map* map, int len, const pointer key );
bool    contains_Map( const Map* map, int len, const pointer key );

// Mutators
pointer      put_Map( Map* map, int len, const pointer key, pointer value );
pointer   remove_Map( Map* map, int len, const pointer key );

#endif
