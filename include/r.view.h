#ifndef __r_view_h__
#define __r_view_h__

#include "math.matrix.h"
#include "r.xform.h"

typedef struct View View;

struct View {

	Xform *lens;
	Xform *eye;

};

mat44 perspective_Lens( float fovy, float aspect, float near, float far );
mat44       ortho_Lens( float left, float right, float bottom, float top );
mat44     compose_Eye( float4 qr, float4 T );

View       define_View( region_p R, mat44 lens, mat44 eye );

#endif
