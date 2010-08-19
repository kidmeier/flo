#include <string.h>

#include "core.alloc.h"

#include "data.hash.h"
#include "data.list.h"
#include "data.map.h"

struct node_s {

	const void* key;
	int         len;
	
	void*       data;
	
	slist_mixin( struct node_s );

};

struct hashmap_s {
	
	float           load;
	float           ideal_load;

	int             n;
	int             s;
	struct node_s** buckets;

};

// Internal ///////////////////////////////////////////////////////////////////

static void expand( hashmap_p map ) {

	struct node_s** old_buckets = map->buckets;

	// Double the number of buckets
	map->buckets    = new_array( map, struct node_s*, 2 * map->s );
	memset( map->buckets, 0, sizeof(struct node_s*) * 2 * map->s );

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
	delete( old_buckets );

	return;
}

static void inc_load( hashmap_p map ) {

	map->n ++;
	map->load = (float)map->n / map->s;

	if( map->load > map->ideal_load )
		expand(map);
	
}

static struct node_s* lookup( const hashmap_p map, int len, const void* key, struct node_s*** bucket ) {

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

hashmap_p new_MAP( float load ) {

	// The initial number of buckets; must be a power of 2
	const int defaultBuckets = 32;

	hashmap_p map = new( NULL, hashmap_t );
	map->load       = 0.0f;
	map->ideal_load = load;
	map->n          = 0;
	map->s          = defaultBuckets;
	map->buckets    = new_array( map, struct node_s*, map->s );
	memset( map->buckets, 0, sizeof(struct node_s*) * map->s );

	return map;

}

void      destroy_MAP( hashmap_p map ) {

	delete( map );

}

void*     put_MAP( hashmap_p map, int len, const void* key, void* data ) {

	struct node_s** bucket = NULL;
	struct node_s*  node = lookup( map, len, key, &bucket );

	// New insertion
	if( !node ) {

		node = new( map, struct node_s );
		node->key = key;
		node->len = len;
		node->data = data;

		slist_push_front( *bucket, node );
		inc_load( map );

		return NULL;

	}

	// Already exists; just update the data
	void* prev = node->data;
	node->data = data;

	return prev;

}

void*     remove_MAP( hashmap_p map, int len, const void* key ) {

	struct node_s** bucket = NULL;
	struct node_s*  node = lookup( map, len, key, &bucket );

	if( !node )
		return NULL;

	void* value = node->data;

	// Peel away the bucket until we find the node
	struct node_s* front = NULL;
	struct node_s* stack = NULL;
	while( front != node ) {

		slist_pop_front( *bucket, front );
		slist_push_front( stack, front );

	}

	// Free the node and push the rest back on the bucket
	slist_pop_front( stack, front );
	delete( front );

	while( stack ) {

		slist_pop_front( stack, front );
		slist_push_front( *bucket, front );

	}

	return value;
	
}

void*     lookup_MAP( const hashmap_p map, int len, const void* key ) {

	struct node_s* node = lookup( map, len, key, NULL );
	return (NULL == node) ? NULL : node->data;

}

bool      contains_MAP( const hashmap_p map, int len, const void* key ) {

	struct node_s* node = lookup( map, len, key, NULL );
	return NULL != node;

}

#ifdef __data_map_TEST__

#include <stdio.h>

int main( int argc, char* argv ) {

	const int N = 10000;

	char** keys   = (char**)malloc( sizeof(char*) * N );
	int*   values = (int*)malloc( sizeof(int) * N );
	hashmap_p M = new_MAP( mapDefaultLoad );

	printf("Insert %d keys into map...\n", N);
	for( int i=0; i<N; i++ ) {

		keys[i] = (char*)malloc( (i / 10 + 2) * sizeof(char) );
		values[i] = i;
		sprintf( keys[i], "%d", i );

		put_MAP( M, strlen(keys[i]), keys[i], &values[i] );

	}

	bool fail = false;
	printf("Ok. Verifying key lookups...\n");
	for( int i=0; i<N; i++ ) {

		void* value = lookup_MAP( M, strlen(keys[i]), keys[i] );
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

	destroy_MAP( M );
	
	return 0;

}

#endif
