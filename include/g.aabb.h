#ifndef __g_aabb_H__
#define __g_aabb_H__

#include <stdbool.h>

#include "math.vec.h"

typedef struct {

	float4 mins;
	float4 maxs;

} AABB;

bool inside_AABB( AABB *aabb, float4 pt );

bool intersects_AABB( AABB *aabb, AABB *other );
bool contains_AABB( AABB *aabb, AABB *other );

void expand_AABB( AABB *aabb, float4 pt );

#endif
