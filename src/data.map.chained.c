#include <string.h>

#include "data.hash.h"
#include "data.list.h"
#include "data.map.h"

#include "mm.region.h"
#include "mm.zone.h"

#define expandThreshold 0.8f

struct node_s {

	pointer key;
	int     len;
	
	pointer data;
	
	slist_mixin( struct node_s );

};

struct Map {

	zone_p          Z;
	region_p        R;

	float           load;

	int             n;
	int             s;
	struct node_s** buckets;

};

// Internal ///////////////////////////////////////////////////////////////////

static void expand( Map* map ) {

	struct node_s** old_buckets = map->buckets;

	// Double the number of buckets
	map->buckets    = zalloc( map->Z, 2 * map->s * sizeof(struct node_s*) );
	memset( map->buckets, 0, 2 * map->s * sizeof(struct node_s*) );

	// Re-hash all the entries
	for( int i=0; i<map->s; i++ ) {

		struct node_s* node; slist_pop_front( old_buckets[i], node );
		while( node ) {
			uint32_t        hash = hashlittle( node->key, node->len, node->len );
			size_t          bucket_idx = hash & (2 * map->s - 1);
			struct node_s** bucket = &map->buckets[ bucket_idx ];

			slist_push_front( *bucket, node );
			slist_pop_front( old_buckets[i], node );
		}

	}

	// Update stats
	map->s          = 2 * map->s;
	map->load = (float)map->n / map->s;
	
	// Free the old buckets
	zfree( map->Z, old_buckets );

	return;
}

static void inc_load( Map* map ) {

	map->n ++;
	map->load = (float)map->n / map->s;

	if( map->load > expandThreshold )
		expand(map);
	
}

static struct node_s* lookup( const Map* map, int len, const pointer key, struct node_s*** bucket ) {

	uint32_t       hash = hashlittle( key, len, len );
	size_t         bucket_idx = hash & (map->s - 1);
	struct node_s* node = map->buckets[ bucket_idx ];

	if( bucket )
		*bucket = &map->buckets[ bucket_idx ];

	// Find the node
	while( NULL != node
	       && (len != node->len 
	           || 0 != memcmp( key, node->key, len )) ) {

		node = slist_next(node);

	}

	return node;

}

// Public API /////////////////////////////////////////////////////////////////

Map* new_Map( zone_p Z, int initial_capacity ) {

	// The initial number of buckets; must be a power of 2
	const int defaultBuckets = 32;

	Map* map = zalloc( Z, sizeof(Map) );
	map->Z = Z;
	map->R = region( Z, "data.map" );
	map->load       = 0.0f;
	map->n          = 0;
	map->s          = (initial_capacity > 0 ? initial_capacity : defaultBuckets);
	map->buckets    = zalloc( Z, map->s * sizeof(struct node_s*) );
	memset( map->buckets, 0, map->s * sizeof(struct node_s*) );

	return map;

}

void      destroy_Map( Map* map ) {

	rfree( map->R );
	zfree( map->Z, map->buckets );
	zfree( map->Z, map );

}

// Functions //////////////////////////////////////////////////////////////////

uint    size_Map( const Map* map ) {

	return map->n;

}

float   load_Map( const Map* map ) {

	return map->load;

}

pointer   lookup_Map( const Map* map, int len, const pointer key ) {

	struct node_s* node = lookup( map, len, key, NULL );
	return (NULL == node) ? NULL : node->data;

}

bool      contains_Map( const Map* map, int len, const pointer key ) {

	struct node_s* node = lookup( map, len, key, NULL );
	return NULL != node;

}

// Mutators ///////////////////////////////////////////////////////////////////

pointer   put_Map( Map* map, int len, const pointer key, pointer data ) {

	struct node_s** bucket = NULL;
	struct node_s*  node = lookup( map, len, key, &bucket );

	// New insertion
	if( !node ) {

		node = ralloc( map->R, sizeof(struct node_s) );
		node->key = key;
		node->len = len;
		node->data = data;

		slist_push_front( *bucket, node );
		inc_load( map );

		return NULL;

	}

	// Already exists; just update the data
	pointer prev = node->data;
	node->data = data;

	return prev;

}

pointer   remove_Map( Map* map, int len, const pointer key ) {

	struct node_s** bucket = NULL;
	struct node_s*  node = lookup( map, len, key, &bucket );

	if( !node )
		return NULL;

	pointer value = node->data;

	// Peel away the bucket until we find the node
	struct node_s* front = NULL;
	struct node_s* stack = NULL;
	while( front != node ) {

		slist_pop_front( *bucket, front );
		slist_push_front( stack, front );

	}

	// Free the node and push the rest back on the bucket
	slist_pop_front( stack, front );

	// TODO: Memory leak. Need to maintain a freelist
//	delete( front );

	while( stack ) {

		slist_pop_front( stack, front );
		slist_push_front( *bucket, front );

	}

	return value;
	
}

#ifdef __data_map_TEST__

#include <stdio.h>

#include "core.log.h"
#include "mm.heap.h"

int main( int argc, char* argv ) {

	const int N = 492*1024;

	set_LOG_level( logFatal );

	char** keys   = (char**)malloc( sizeof(char*) * N );
	int*   values = (int*)malloc( sizeof(int) * N );
	Map* M = new_Map( ZONE_heap, 1024*1024 );

	printf("Insert %d keys into map...\n", N);
	for( int i=0; i<N; i++ ) {

		keys[i] = (char*)malloc( ((int)ceil(log10(i)) + 2) * sizeof(char) );
//		keys[i] = (char*)malloc( (i / 10 + 2) * sizeof(char) );
		values[i] = i;
		sprintf( keys[i], "%d", i );

		put_Map( M, strlen(keys[i]), keys[i], &values[i] );

	}

	bool fail = false;
	printf("Ok. Verifying key lookups...\n");
	for( int i=0; i<N; i++ ) {

		pointer value = lookup_Map( M, strlen(keys[i]), keys[i] );
		if( value != &values[i] ) {
			printf(" FAIL: 0x%x *(%d) != 0x%x *(%d)\n", 
			       *(int*)value, (unsigned)value, values[i], (unsigned)&values[i]);
			fail = true;
		}

	}
	if( fail ) 
		printf("Oops; some lookups returned incorrect values.\n");
	else
		printf("Great, all the lookups returned the expected value.\n");

	printf("Stats:\n");
	printf("  N:    %d\n", M->n);
	printf("  S:    %d\n", M->s);
	printf("  Load: %f\n", M->load);

	destroy_Map( M );
	
	return 0;

}

#endif
