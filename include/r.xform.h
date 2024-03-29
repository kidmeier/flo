#ifndef __r_xform_h__
#define __r_xform_h__

#include "core.types.h"
#include "math.matrix.h"
#include "math.vec.h"
#include "mm.region.h"

// Opaque transform type
typedef struct Xform Xform;

// Visiter function type
typedef bool (*xformVisitor_f)( const Xform*, pointer tag );

// Allocation /////////////////////////////////////////////////////////////////
Xform          *new_Xform( region_p R, Xform* parent, pointer tag );
Xform          *new_Xform_scale( region_p R, Xform *parent, pointer tag, float4 scale );
Xform          *new_Xform_tr( region_p R, Xform *parent, pointer tag, float4 tr );
Xform          *new_Xform_qr( region_p R, Xform *parent, pointer tag, float4 qr );
Xform          *new_Xform_qr_tr( region_p R, Xform *parent, pointer tag, float4 qr, float4 tr );
Xform          *new_Xform_scale_qr_tr( region_p R, Xform *parent, pointer tag,
                                       float4 scale, float4 qr, float4 tr );
Xform          *new_Xform_m( region_p R, Xform *parent, pointer tag, const mat44 *m );

// Tree manipulation //////////////////////////////////////////////////////////

Xform        *adopt_Xform( Xform *xf, Xform *child );
Xform       *orphan_Xform( Xform *xf, Xform *child );

Xform       *attach_Xform( Xform *xf, Xform *parent );
Xform       *detach_Xform( Xform *xf, Xform *parent );

// Functions //////////////////////////////////////////////////////////////////
Xform       *parent_Xform( const Xform *xf );
pointer         tag_Xform( const Xform *xf );

void       traverse_Xform( const Xform *xf, xformVisitor_f visit );

// Mutators ///////////////////////////////////////////////////////////////////
Xform          *set_Xform( Xform *xf, const mat44 *t );
Xform          *mul_Xform( Xform *xf, const mat44 *m );
Xform        *scale_Xform( Xform *xf, float4 scale );
Xform       *rotate_Xform( Xform *xf, float4 qr );
Xform    *translate_Xform( Xform *xf, float4 v );

// Transforms /////////////////////////////////////////////////////////////////
const mat44 *object_Xform( Xform *xf );
const mat44  *world_Xform( Xform *xf );
mat44     worldview_Xform( Xform *view, Xform *world );

// Inverses ///////////////////////////////////////////////////////////////////
const mat44 *object_Xform_1( Xform *xf );
const mat44  *world_Xform_1( Xform *xf );
mat44     worldview_Xform_1( Xform *view, Xform *world );

#endif
