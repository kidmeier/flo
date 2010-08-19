#ifndef __data_map_h__
#define __data_map_h__

typedef struct hashmap_s hashmap_t;
typedef hashmap_t* hashmap_p;

#define mapDefaultLoad    0.7f

hashmap_p new_MAP( float load );
void      destroy_MAP( hashmap_p map );

void*     put_MAP( hashmap_p map, int len, const void* key, void* data );
void*     remove_MAP( hashmap_p map, int len, const void* key );

void*     lookup_MAP( const hashmap_p map, int len, const void* key );
bool      contains_MAP( const hashmap_p map, int len, const void* key );

#endif
