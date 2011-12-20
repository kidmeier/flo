#include "core.features.h"
#include "core.types.h"
#include "data.list.h"
#include "mm.region.h"
#include "r.xform.h"

typedef enum { 

	worldDirty   = 0x01,
	world_1Dirty  = 0x02,
	object_1Dirty = 0x04,
	
	allDirty     = 0xFF,

} xformDirty_e;

struct Xform  {

	Xform*  parent;
	List*   children;
	pointer tag;

	byte    dirty;

	mat44   object;
	mat44   world;
	mat44   object_;
	mat44   world_;
	
};

static void mark( Xform* xf, uint flags ) {

	xf->dirty |= flags;

}

static void mark_children( Xform* xf, uint flags ) {

	// Mark children dirty
	for( Xform* child=first_List(xf->children);
	     NULL != child;
	     child = next_List( child ) ) {

		mark( child, flags );

	}

}

static void clean( Xform* xf, uint flags ) {

	xf->dirty &= ~(flags);

}

static bool isdirty( Xform* xf, uint flags ) {

	return 0 != (xf->dirty & flags);

}

// Public /////////////////////////////////////////////////////////////////////

Xform*          new_Xform( region_p R, Xform* parent, pointer tag ) {
	
	return new_Xform_tr( R, parent, tag, &identity_MAT44 );

}

Xform*          new_Xform_scale( region_p R,
                                 Xform*   parent, 
                                 pointer  tag,
                                 float4   scale ) {

	// Sc . v
	mat44 M = mscaling( scale );
	return new_Xform_tr( R, parent, tag, &M );

}

Xform*          new_Xform_qr( region_p R, 
                              Xform*   parent, 
                              pointer  tag,
                              float4   qr ) {

	// Rot . v
	mat44 M = qmatrix(qr);
	return new_Xform_tr( R, parent, tag, &M );

}

Xform*          new_Xform_qr_tr( region_p R, 
                                 Xform*   parent, 
                                 pointer  tag,
                                 float4   qr,
                                 float4   tr ) {

	// Tr . Rot . v
	mat44 M = mmul( mtranslation(tr), qmatrix(qr) );
	return new_Xform_tr( R, parent, tag, &M );

}

Xform*          new_Xform_scale_qr_tr( region_p R,
                                       Xform*   parent, 
                                       pointer  tag,
                                       float4   scale, 
                                       float4   qr, 
                                       float4   tr ) {

	// Tr . Rot . Sc . v
	mat44 M = mmul( mmul( mtranslation(tr), qmatrix(qr) ), mscaling( scale ) );	
	return new_Xform_tr( R, parent, tag, &M );

}

Xform*          new_Xform_tr( region_p R,
                              Xform*   parent, 
                              pointer  tag,
                              const mat44* tr ) {

	
	Xform* xf = NULL;
	
	if( parent ) { 
		
		xf = new_List_item( parent->children );
		push_back_List( parent->children, xf );
		
	} else {
		
		xf = ralloc( R, sizeof(Xform) );
		
	}
	
	xf->parent   = parent;
	xf->children = new_List( R, sizeof(Xform) );
	xf->tag      = tag;

	xf->object   = *(tr);
	mark( xf, object_1Dirty | worldDirty | world_1Dirty );

	return xf;
	
}

// Tree manipulation //////////////////////////////////////////////////////////

Xform*        adopt_Xform( Xform* xf, Xform* child ) {

	child->parent = xf;
	push_back_List( xf->children, child );

	return xf;;

}

Xform*       orphan_Xform( Xform* xf, Xform* child ) {

	// Make sure child is in our list of children
#if defined(feature_DEBUG)
	Xform* node;
	find__List( xf->children, node, node == child );

	assert( NULL != node );
#endif

	remove_List( xf->children, child );
	child->parent = NULL;

	return xf;

}

Xform*       attach_Xform( Xform* xf, Xform* parent ) {

	xf->parent = parent;
	push_back_List( parent->children, xf );

	return xf;

}

Xform*       detach_Xform( Xform* xf, Xform* parent ) {

	// Make sure we are in parent's list of children
#if defined(feature_DEBUG)
	Xform* node;
	find__List( parent->children, node, node == xf );

	assert( NULL != node );
#endif

	remove_List( parent->children, xf );
	xf->parent = NULL;
	
	return xf;

}

// Functions //////////////////////////////////////////////////////////////////

Xform*       parent_Xform( const Xform* xf ) {

	return xf->parent;

}

pointer         tag_Xform( const Xform* xf ) {

	return xf->tag;

}

void       traverse_Xform( const Xform* xf, xformVisitor_f visit ) {

	
	if( visit( xf, xf->tag ) ) {

		for( Xform* child=first_List(xf->children);
		     NULL != child;
		     child = next_List( xf->children ) )
			
			traverse_Xform( child, visit );

	}

}

// Mutators ///////////////////////////////////////////////////////////////////
Xform*          set_Xform( Xform* xf, const mat44* t ) {

	xf->object = *t;

	mark( xf, object_1Dirty | worldDirty | world_1Dirty );
	mark_children( xf, worldDirty | world_1Dirty );

	return xf;

}

Xform*          mul_Xform( Xform* xf, const mat44* t ) {

	mat44 tr = mmulv( &xf->object, t );
	return set_Xform( xf, &tr );

}


Xform*        scale_Xform( Xform* xf, float4 scale ) {

	mat44 tr = mmul( xf->object, mscaling(scale) );
	return set_Xform( xf, &tr );

}

Xform*       rotate_Xform( Xform* xf, float4 qr ) {

	mat44 tr = mmul( xf->object, qmatrix(qr) );
	return set_Xform( xf, &tr );

}

Xform*    translate_Xform( Xform* xf, float4 v ) {

	mat44 tr = mmul( xf->object, mtranslation(v) );
	return set_Xform( xf, &tr );

}

const mat44* object_Xform( Xform* xform ) {

	return &xform->object;

}

const mat44*  world_Xform( Xform* xform ) {

	if( isdirty( xform, worldDirty ) ) {
		
		// If we have a parent, we pre-multiply its transform with our own
		if( xform->parent ) {

			xform->world = mmulv(world_Xform(xform->parent), 
			                     object_Xform(xform));

		} else
			// We are the top, just return our transform
			xform->world = *(object_Xform(xform));


		clean( xform, worldDirty );

	}

	return &xform->world;

}

mat44        worldview_Xform( Xform* view, Xform* xform ) {

	return mmulv(world_Xform(view), world_Xform(xform));

}

const mat44* object_Xform_1( Xform* xform ) {

	if( isdirty(xform, object_1Dirty) ) {

		xform->object_ = minverse33( xform->object );
		clean( xform, object_1Dirty);

	}

	return &xform->object_;

}

const mat44*  world_Xform_1( Xform* xform ) {

	if( isdirty(xform, world_1Dirty) ) {

		// If we have a parent, we pre-multiply its inverse with our own
		if( xform->parent ) {

			xform->world_ = mmulv( object_Xform_1(xform), 
			                       world_Xform_1(xform->parent) );
	   
		} else // We are the top, just return our transform

			xform->world_ = *object_Xform_1(xform);

		clean( xform, world_1Dirty );

	}

	return &xform->world_;

}

mat44     worldview_Xform_1( Xform* view, Xform* xform ) {

	return mmulv(world_Xform_1(xform), world_Xform_1(view));
	
}

#ifdef __r_xform_TEST__

#include <stdio.h>

int main( int argc, char* argv[] ) {

	init_printf_MAT44();

	region_p R = region( "r.xform.TEST" );

	const float pi6 = M_PI / 6.f;

	// Identity
	Xform* root = new_Xform_qr_tr( R, NULL, NULL,
	                              (float4){ 0.f, 0.f, 0.f, 1.f }, 
	                              (float4){ 0.f, 0.f, 0.f, 1.f } );
	Xform* X = new_Xform_qr_tr( R, root, NULL,
	                           qaxis((float4){ 1.f, 0.f, 0.f, pi6 }), 
	                           (float4){ 0.f, 0.f, 0.f, 1.f } );
	Xform* Y = new_Xform_qr_tr( R, X, NULL,
	                           qaxis((float4){ 0.f, 1.f, 0.f, pi6 }), 
	                           (float4){ 0.f, 0.f, 0.f, 1.f } );
	Xform* Z = new_Xform_qr_tr( R, X, NULL,
	                           qaxis((float4){ 0.f, 0.f, 1.f, pi6 }), 
	                           (float4){ 0.f, 0.f, 0.f, 1.f } );
	
	const mat44* rootM = world_Xform(root);

	const mat44*    oX = object_Xform(X);
	const mat44*    oY = object_Xform(Y);
	const mat44*    oZ = object_Xform(Z);
	const mat44*   ioX = object_Xform_1(X);
	const mat44*   ioY = object_Xform_1(Y);
	const mat44*   ioZ = object_Xform_1(Z);
	
	const mat44*    wX = world_Xform(X);
	const mat44*    wY = world_Xform(Y);
	const mat44*    wZ = world_Xform(Z);
	const mat44*   iwX = world_Xform_1(X);
	const mat44*   iwY = world_Xform_1(Y);
	const mat44*   iwZ = world_Xform_1(Z);

	const mat44 IoX = mmulv(oX,ioX);
	const mat44 IoY = mmulv(oY,ioY);
	const mat44 IoZ = mmulv(oZ,ioZ);
	const mat44 IwX = mmulv(wX,iwX);
	const mat44 IwY = mmulv(wY,iwY);
	const mat44 IwZ = mmulv(wZ,iwZ);

	printf("world(root) = % #4.2M\n", rootM);

	printf("  object(X) = % #4.2M\n", oX);
	printf("  object(Y) = % #4.2M\n", oY);
	printf("  object(Z) = % #4.2M\n", oZ);
	printf("   world(X) = % #4.2M\n", wX);
	printf("   world(Y) = % #4.2M\n", wY);
	printf("   world(Z) = % #4.2M\n", wZ);

	printf("object(X) * object_1(X) = % #4.2M\n", &IoX);
	printf("object(Y) * object_1(Y) = % #4.2M\n", &IoY);
	printf("object(Z) * object_1(Z) = % #4.2M\n", &IoZ);
	printf("  world(X) * world_1(X) = % #4.2M\n", &IwX);
	printf("  world(Y) * world_1(Y) = % #4.2M\n", &IwY);
	printf("  world(Z) * world_1(Z) = % #4.2M\n", &IwZ);

	rcollect( R );

}

#endif
