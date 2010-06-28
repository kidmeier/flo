#ifndef __r_xform_h__
#define __r_xform_h__

#include "math.matrix.h"
#include "math.vec.h"

// Opaque transform type
struct xform_s;
typedef struct xform_s xform_t;
typedef xform_t* xform_p;

xform_p attach_XFORM( const xform_p parent, const float4 qr, const float4 v);
xform_p parent_XFORM( const xform_p xform );

mat44   object_XFORM( const xform_p xform );
mat44   world_XFORM( const xform_p xform );
mat44   worldview_XFORM( const xform_p view, const xform_p xform );

mat44   invobject_XFORM( const xform_p xform );
mat44   invworld_XFORM( const xform_p xform );
mat44   invworldview_XFORM( const xform_p view, const xform_p xform );

#endif
