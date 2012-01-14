#ifndef __r_scene_h__
#define __r_scene_h__

#include "control.predicate.h"
#include "core.types.h"
#include "data.map.h"
#include "gl.shader.h"
#include "mm.region.h"
#include "r.drawable.h"
#include "r.xform.h"

typedef struct Scene Scene;
typedef struct Visual Visual;

Scene*  new_Scene( region_p R );

void    draw_Scene( float t0, float t, float dt, Scene* sc, uint32 pass, predicate_f cull );

Visual* link_Scene( Scene      *sc, 
                    pointer     tag, 
                    uint32      mask, 
                    Drawable   *drawable,
                    int         argc,
                    Shader_Arg *argv );

void  unlink_Scene( Scene *sc, Visual *vis );

#endif
