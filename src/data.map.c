#include <assert.h>
#include <math.h>
#include <string.h>

#include "core.types.h"

#include "data.hash.h"
#include "data.map.h"

#include "math.util.h"

#include "mm.zone.h"

#define  H         32
#define  PROBEDIST 256

struct Bucket {

	uint    neighbourhood;
	
	pointer key;
	int     len;
	
	pointer value;

};

struct Map {

	zone_p         Z;

	uint           N;
	uint           S;
	struct Bucket* buckets;

};

// Internal ///////////////////////////////////////////////////////////////////

static inline int find_lsb_set( uint mask ) {
	
	for( int i=0; i<H; i++ ) {
		if( mask & (1U << i) )
			return i;
	}

	return -1;

}

static inline bool eqkey( const pointer k1, int l1, const pointer k2, int l2 ) {

	return (l1 == l2)
		&& (NULL != k1 && NULL != k2)
		&& (0 == memcmp( k1, k2, l1 ));

}

static inline bool isempty( struct Bucket* bucket ) {

	return NULL == bucket->key;

}

/*
static void expand( Map* map ) {

	struct Bucket** old_buckets = map->buckets;

	// Double the number of buckets
	map->buckets    = new_array( map, struct Bucket*, 2 * map->S );
	memset( map->buckets, 0, sizeof(struct Bucket*) * 2 * map->S );

	// Re-hash all the entries
	for( int i=0; i<map->S; i++ ) {

		struct Bucket* node; slist_pop_front( old_buckets[i], node );
		while( node ) {
			uint32_t        hash = hashlittle( node->key, node->len, node->len );
			size_t          bucket_idx = hash & (2 * map->S - 1);
			struct Bucket** bucket = &map->buckets[ bucket_idx ];

			slist_push_front( *bucket, node );
			slist_pop_front( old_buckets[i], node );
		}

	}

	// Update stats
	map->S          = 2 * map->S;
	map->load = (float)map->N / map->S;
	
	// Free the old buckets
	delete( old_buckets );

	return;
}
*/

static struct Bucket* lookup( const Map* map, int len, const pointer key ) {

	uint32_t       hash = hashlittle( key, len, len );
	size_t         base_idx = hash & (map->S - 1);
	struct Bucket* base = &map->buckets[ base_idx ];

	// Is this a worthwhile optimization?
//	if( 1 == base->neighbourhood 
//	    && eqkey( key, len, base->key, base->len ) )
//		return base;

	// Check buckets in this neighbourhood
	uint mask = base->neighbourhood;
	while( mask ) {
		int i = find_lsb_set( mask );
		if( i < 0 )
			break;
		struct Bucket* bucket = &map->buckets[ (base_idx + i) & (map->S-1) ];

		assert( i <= 31 );

		if( eqkey( key, len, bucket->key, bucket->len ) )
			return bucket;

		mask &= ~(1U << i);
	}
	
	// Nope
	return NULL;

}

// This is where the magic happens. The goal is find the minimal bucket
// within a distance of H-1 from `freebucket` that can be moved to 
// `freebucket` without breaking its neighbourhood property
//static struct Bucket* pull( struct Bucket* freebucket ) {
static int pull( struct Bucket* buckets, uint S, uint freebucket_idx ) {

	struct Bucket* freebucket = &buckets[ freebucket_idx ];

	int base_idx = freebucket_idx - (H-1); // HOW SURE AM I THAT freebucket_idx is always correct?
	if( base_idx < 0 )
		base_idx = S + base_idx;
	
	for( int freedist=H-1; freedist>0; freedist-- ) {

		struct Bucket* base = &buckets[ base_idx ];

		assert( freedist <= 31 );
		assert( base != freebucket );

		// Look for a bucket before `freebucket` and in `base`'s neighbourhood
		// that belongs in `base`'s bucket
		for( int i=0; i<freedist; i++ ) {
			
			assert( i <= 31 );

			// Found one?
			if( (1U << i) & base->neighbourhood ) {

				const int target_idx = (base_idx + i) & (S-1);
				struct Bucket* target = &buckets[ target_idx ];

				// Mark the free location as used
				base->neighbourhood |= (1U << freedist);
				
				// Move the target's data into the free bucket
				freebucket->key = target->key;
				freebucket->len = target->len;
				freebucket->value = target->value;
				
				// Free the old location
				base->neighbourhood &= ~(1U << i);
				target->key = NULL;
				target->len = 0;
				target->value = NULL;

				return target_idx;
				
			}
				
		}

		// Try next
		base_idx = (base_idx + 1) & (S-1);

	}

	// Didn't find anything
	return -1;

}

// It is an error to call this function when `0 != lookup(map, key, len)`
// Calling code should check the existence of the key before calling this.
static struct Bucket* put( Map* map, int len, const pointer key, pointer value ) {

	uint32_t       hash = hashlittle( key, len, len );
	size_t         base_idx = hash & (map->S - 1);
	struct Bucket* base = &map->buckets[ base_idx ];

	// Probe for an empty slot
	for( size_t i=0; i<PROBEDIST; i++ ) {

		int bucket_idx = (base_idx + i) & (map->S-1);
		struct Bucket* bucket = &map->buckets[ bucket_idx ];
		if( isempty(bucket) ) {

			do {

				bucket = &map->buckets[ bucket_idx ];

				// Found a free bucket in the neighbourhood
				int dist = bucket_idx - base_idx;
				if( dist < 0 )
					dist = map->S + dist;

				if( dist < H ) {
					
					base->neighbourhood |= (1U << dist);
					bucket->key = key;
					bucket->len = len;
					bucket->value = value;
					
					map->N++;
					return bucket;

				}
				// Too far, try and pull it closer
				bucket_idx = pull( map->buckets, map->S, bucket_idx );

			} while( bucket_idx > 0 );

			// Fail
			return NULL;

		}

	}

	return NULL;

}

// Instantiations /////////////////////////////////////////////////////////////

Map* new_Map( zone_p Z, int capacity ) {

	// The initial number of buckets; must be a power of 2
	const int defaultBuckets = 32;

	Map* map = zalloc( Z, sizeof(Map) );
	map->Z       = Z;
	map->N       = 0;
	map->S       = ceil2u( capacity > 0 ? capacity : defaultBuckets );
	map->buckets = zalloc( Z, map->S * sizeof(struct Bucket) );
	memset( map->buckets, 0, map->S * sizeof(struct Bucket) );

	return map;

}

void      destroy_Map( Map* map ) {

	zfree( map->Z, map->buckets );
	zfree( map->Z, map );

}

// Functions //////////////////////////////////////////////////////////////////

uint        size_Map( const Map* map ) {

	return map->N;

}

float       load_Map( const Map* map ) {

	return (float)map->N / map->S;

}

pointer     lookup_Map( const Map* map, int len, const pointer key ) {

	struct Bucket* node = lookup( map, len, key );
	return (NULL == node) ? NULL : node->value;

}

bool      contains_Map( const Map* map, int len, const pointer key ) {

	struct Bucket* node = lookup( map, len, key );
	return NULL != node;

}

// Mutators ///////////////////////////////////////////////////////////////////

pointer     put_Map( Map* map, int len, const pointer key, pointer value ) {

	struct Bucket* node = lookup( map, len, key );

	// Replace
	if( node ) {

		pointer old = node->value;

		node->key = key;
		node->len = len;
		node->value = value;

		return old;

	}

	// Insert new
	node = put( map, len, key, value );
	if( !node )
		return NULL;
	
	return node->value;

}

pointer     remove_Map( Map* map, int len, const pointer key ) {

	struct Bucket*  node = lookup( map, len, key );

	if( !node )
		return NULL;

	pointer old = node->value;

	node->key = NULL;
	node->len = 0;
	node->value = NULL;

	return old;

}

#ifdef __data_map_TEST__

#include <stdio.h>
#include <stdlib.h>

#include "mm.heap.h"

int main( int argc, char* argv[] ) {

	const int N = 492*1024;

	char** keys   = malloc( sizeof(char*) * N );
	int*   values = malloc( sizeof(int) * N );
	Map* M = new_Map( ZONE_heap, 1024*1024);

	printf("Insert %d keys into map...\n", N);
	for( int i=0; i<N; i++ ) {

		int keyLength = (int)ceil(log10(1.0 + (double)i)) + 2.0;
		keys[i] = (char*)malloc( keyLength * sizeof(char) );
		values[i] = i;
		sprintf( keys[i], "%d", i );

		if( &values[i] != put_Map( M, strlen(keys[i]), keys[i], &values[i] ) )
			fprintf(stderr, "FAILED to insert %s=%d\n", keys[i], values[i]);

	}

	bool fail = false;
	printf("Ok. Verifying key lookups...\n");
	for( int i=0; i<N; i++ ) {

		pointer value = lookup_Map( M, strlen(keys[i]), keys[i] );
		if( value != &values[i] ) {
			printf(" FAIL: 0x%x *(%p) != 0x%x *(%p)\n", 
			       *(int*)value, value, values[i], &values[i]);
			fail = true;
		}

	}
	if( fail ) 
		printf("Oops; some lookups returned incorrect values.\n");
	else
		printf("Great, all the lookups returned the expected value.\n");

	printf("Stats:\n");
	printf("  N:    %d\n", size_Map(M));
	printf("  S:    %d\n", M->S);
	printf("  Load: %f\n", load_Map(M));

	destroy_Map( M );
	
	return 0;

}

#endif
