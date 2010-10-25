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
Xform*          new_Xform( region_p R, Xform* parent, pointer tag );
Xform*          new_Xform_scale( region_p R, 
                                 Xform* parent, 
                                 pointer tag,
                                 float4 scale );
Xform*          new_Xform_qr( region_p R, 
                              Xform*   parent, 
                              pointer  tag,
                              float4   qr );
Xform*          new_Xform_qr_v( region_p R, 
                                Xform*   parent, 
                                pointer  tag,
                                float4   qr,
                                float4   v);
Xform*          new_Xform_scale_qr_v( region_p R, 
                                      Xform*   parent, 
                                      pointer  tag,
                                      float4   scale,
                                      float4   qr,
                                      float4   v);

Xform*          new_Xform_tr( region_p R,
                              Xform*   parent, 
                              pointer  tag,
                              const mat44* tr );

// Tree manipulation //////////////////////////////////////////////////////////

Xform*        adopt_Xform( Xform* xf, Xform* child );
Xform*       orphan_Xform( Xform* xf, Xform* child );

Xform*       attach_Xform( Xform* xf, Xform* parent );
Xform*       detach_Xform( Xform* xf, Xform* parent );

// Functions //////////////////////////////////////////////////////////////////
Xform*       parent_Xform( const Xform* );
pointer         tag_Xform( const Xform* );

void       traverse_Xform( const Xform*, xformVisitor_f f );

// Mutators ///////////////////////////////////////////////////////////////////
Xform*          set_Xform( Xform*, const mat44* t );
Xform*          mul_Xform( Xform*, const mat44* m );
Xform*        scale_Xform( Xform*, float4 scale );
Xform*       rotate_Xform( Xform*, float4 qr );
Xform*    translate_Xform( Xform*, float4 v );

// Transforms /////////////////////////////////////////////////////////////////
const mat44* object_Xform( Xform* xform );
const mat44*  world_Xform( Xform* xform );
mat44     worldview_Xform( Xform* view, Xform* xform );

// Inverses ///////////////////////////////////////////////////////////////////
const mat44* object_Xform_1( Xform* xform );
const mat44*  world_Xform_1( Xform* xform );
mat44     worldview_Xform_1( Xform* view, Xform* xform );

#endif
