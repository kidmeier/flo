#include "g.aabb.h"
#include "math.util.h"

bool inside_AABB( AABB *aabb, float4 pt ) {

	return ( aabb->mins.x <= pt.x && pt.x < aabb->maxs.x &&
	         aabb->mins.y <= pt.y && pt.y < aabb->maxs.y &&
	         aabb->mins.z <= pt.z && pt.z < aabb->maxs.z );

}

bool intersects_AABB( AABB *aabb, AABB *other ) {

	return inside_AABB( aabb, other->mins ) 
		|| inside_AABB( aabb, other->maxs ) 
		|| contains_AABB( other, aabb );

}

bool contains_AABB( AABB *aabb, AABB *other ) {

	return inside_AABB( aabb, other->mins ) && inside_AABB( aabb, other->maxs );

}

void expand_AABB( AABB *aabb, float4 pt ) {

	aabb->mins = (float4){ 
		min( aabb->mins.x, pt.x ),
		min( aabb->mins.y, pt.y ),
		min( aabb->mins.z, pt.z ),
		1.f
	};

	aabb->maxs = (float4){ 
		max( aabb->maxs.x, pt.x ),
		max( aabb->maxs.y, pt.y ),
		max( aabb->maxs.z, pt.z ),
		1.f
	};

}
