#include "core.alloc.h"
#include "r.xform.h"

struct xform_s  {

	struct xform_s* parent;

	float4 qr; // orientation
	float4 v;  // position

};

xform_p attach_XFORM( const xform_p parent, const float4 qr, const float4 v) {

	xform_p xf = new(parent,xform_t);

	xf->parent = parent;
	xf->qr = qr;
	xf->v = v;

	return xf;
}

xform_p parent_XFORM( const xform_p xform ) {
	return xform->parent;
}

mat44   object_XFORM( const xform_p xform ) {

	mat44 M = qmatrix(xform->qr);
	M._4 = xform->v;
	return M;

}

mat44   world_XFORM( const xform_p xform ) {

	// If we have a parent, we pre-multiply its transform with our own
	if( xform->parent )
		return mmul(world_XFORM(xform->parent), object_XFORM(xform));
	else // We are the top, just return our transform
		return object_XFORM(xform);

}

mat44   worldview_XFORM( const xform_p view, const xform_p xform ) {

	return mmul(world_XFORM(view), world_XFORM(xform));

}

mat44   invobject_XFORM( const xform_p xform ) {

	// From Real-time Rendering pg.62
	float4 vneg = xform->v;
	vneg.x = -vneg.x;
	vneg.y = -vneg.y;
	vneg.z = -vneg.z;

	return mmul( mtranspose(qmatrix(xform->qr)), mtranslate(vneg) );

}

mat44   invworld_XFORM( const xform_p xform ) {

	// If we have a parent, we pre-multiply its transform with our own
	if( xform->parent )
		return mmul(invobject_XFORM(xform), invworld_XFORM(xform->parent));
	else // We are the top, just return our transform
		return invobject_XFORM(xform);	

}

mat44   invworldview_XFORM( const xform_p view, const xform_p xform ) {

	return mmul(invworld_XFORM(xform), invworld_XFORM(view));

}

#ifdef __r_xform_TEST__

int main( int argc, char* argv[] ) {

	init_printf_MAT44();

	// Identity
	const float pi6 = M_PI / 6.f;
	const xform_p root = attach_XFORM(NULL, 
	                                  (float4){ 0.f, 0.f, 0.f, 1.f }, 
	                                  (float4){ 0.f, 0.f, 0.f, 1.f });
	const xform_p X = attach_XFORM(root, 
	                               qaxis((float4){ 1.f, 0.f, 0.f, pi6 }), 
	                               (float4){ 1.f, 1.f, 1.f, 1.f });
	const xform_p Y = attach_XFORM(X, 
	                               qaxis((float4){ 0.f, 1.f, 0.f, pi6 }), 
	                               (float4){ 0.f, 0.f, 0.f, 1.f });

	const mat44 rootM = world_XFORM(root);

	const mat44 oX = object_XFORM(X);
	const mat44 oY = object_XFORM(Y);
	const mat44 ioX = invobject_XFORM(X);
	const mat44 ioY = invobject_XFORM(Y);

	const mat44 wX = world_XFORM(X);
	const mat44 wY = world_XFORM(Y);
	const mat44 iwX = invworld_XFORM(X);
	const mat44 iwY = invworld_XFORM(Y);

	const mat44 IoX = mmul(oX,ioX);
	const mat44 IoY = mmul(oY,ioY);
	const mat44 IwX = mmul(wX,iwX);
	const mat44 IwY = mmul(wY,iwY);

	printf("world(root) = % #4.2M\n", &rootM);
	printf("object(X) = % #4.2M\n", &oX);
	printf("object(Y) = % #4.2M\n", &oY);
	printf("world(X) = % #4.2M\n", &wX);
	printf("world(Y) = % #4.2M\n", &wY);

	printf("object(X)*invobject(X) = % #4.2M\n", &IoX);
	printf("object(Y)*invobject(Y) = % #4.2M\n", &IoY);
	printf("world(X)*invworld(X) = % #4.2M\n", &IwX);
	printf("world(Y)*invworld(Y) = % #4.2M\n", &IwY);

}

#endif
