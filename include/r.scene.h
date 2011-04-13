#ifndef __r_scene_h__
#define __r_scene_h__

#include "core.types.h"
#include "data.map.h"
#include "gl.shader.h"
#include "mm.region.h"
#include "r.drawable.h"
#include "r.xform.h"

typedef struct Scene Scene;

struct Scene {

	region_p R;
	Map*     buckets;

};

typedef struct Visual Visual;

Scene*  new_Scene( region_p R );

void    draw_Scene( float t0, float t, float dt, Scene* sc, uint32 pass, predicate_f cull );

Visual* link_Scene( Scene*, pointer tag, uint32, Drawable*, int, Shader_Arg* );
void  unlink_Scene( Scene*, Visual* );

#endif
