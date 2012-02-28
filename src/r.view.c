#include <math.h>
#include <stdio.h>

#include "gl.shader.h"
#include "math.util.h"
#include "r.view.h"

mat44 perspective_Lens( float fovy, float aspect, float near, float far ) {

	float x = near * tanf( .5f * deg2rad( aspect * fovy ) );
	float y = near * tanf( .5f * deg2rad( fovy ) );

	return mfrustum( -x, x, -y, y, near, far );

}

mat44       ortho_Lens( float left, float right, float bottom, float top ) {

	return mortho( left, right, bottom, top, -1.f, 1.f );

}

mat44     compose_Eye( float4 qr, float4 T ) {

	return mmul( qmatrix(qr), mtranslation(vneg(T)) );

}

View       define_View( mat44 lens, mat44 eye ) {

	return (View){ .lens = lens, .eye = eye };

}

