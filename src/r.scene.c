#include <assert.h>

#include "data.vector.h"
#include "mm.heap.h"
#include "r.scene.h"

struct Visual {

	pointer     tag;

	uint32      mask;
	Drawable*   draw;

	int         argc;
	Shader_Arg* argv;

	Visual*     next;

};

struct Bucket {

	Vector*   visuals;
	Visual*   freelist;

};

static const int hashSize   = 1024;
static const int bucketSize = 1024;

Scene*  new_Scene( region_p R ) {

	Scene* sc = ralloc( R, sizeof(Scene) );

	sc->R       = R;
	sc->buckets = new_Map( ZONE_heap, hashSize );

	return sc;

}

void   draw_Scene( float t0, float t, float dt, Scene* sc, uint32 pass, predicate_f cull ) {

	// 1. For each bucket
	for( pointer kv=first_Map( sc->buckets );
	     NULL != kv;
	     kv = next_Map( sc->buckets, kv ) ) {

		struct Bucket* bucket = (struct Bucket*)value_Map(kv);

		// 2. Is this bucket culled?
		if( cull( key_Map(kv) ) )
			continue;

		// 3. Render each drawable in the bucket
		for( int i=0; i<size_Vector(bucket->visuals); i++ ) {

			Visual* vis = (Visual*)nth_Vector( bucket->visuals, i );

			// Does the visual participate in this pass?
			if( !(vis->mask & pass) )
				continue;

			draw_Drawable( vis->draw, vis->argc, vis->argv );

		}

	}

}

Visual* link_Scene( Scene*      sc, 
                    pointer     tag, 
                    uint32      mask,
                    Drawable*   dr, 
                    int         argc,
                    Shader_Arg* argv ) {

	struct Bucket* bucket = lookup_Map( sc->buckets, 
	                            sizeof( tag ),
	                                   &tag );
	if( NULL == bucket ) {

		// Create a new bucket for this Program
		struct Bucket* bucket = ralloc( sc->R, sizeof(struct Bucket) );

		bucket->visuals  = new_Vector( ZONE_heap, sizeof(Visual), bucketSize );
		bucket->freelist = NULL;

		put_Map( sc->buckets, sizeof(tag), &tag, bucket );

	}

	// Stick it in the bucket
	Visual* vis = NULL;
	if( bucket->freelist ) {

		vis              = bucket->freelist;
		bucket->freelist = vis->next;

	} else 
		vis = (Visual*)push_back_Vector( bucket->visuals );

	vis->tag  = tag;
	vis->mask = mask;
	vis->draw = dr;
	vis->argc = argc;
	vis->argv = argv;
	vis->next = NULL;

	return vis;

}

void unlink_Scene( Scene* sc, Visual* vis ) {

	assert( NULL != sc && NULL != vis );

	struct Bucket* bucket = lookup_Map( sc->buckets,
	                            sizeof( vis->tag ),
	                                   &vis->tag );
	
	// It is an error to unlink something that was never in the scene
	assert( NULL != bucket );
	assert( 0 >= index_of_Vector( bucket->visuals, vis ) );

	vis->tag  = NULL;
	vis->draw = NULL;
	vis->argc = 0;
	vis->argv = NULL;

	vis->next        = bucket->freelist;
	bucket->freelist = vis;

}
