#include <math.h>
#include <stdio.h>

#include "gl.shader.h"
#include "r.view.h"

static inline float deg2rad( float x ) {
	return ( M_PI * x / 180.f );
}

mat44 perspective_View( float fovy, float aspect, float near, float far ) {

	float x = near * tanf( .5f * deg2rad( aspect * fovy ) );
	float y = near * tanf( .5f * deg2rad( fovy ) );

	glMatrixMode( GL_PROJECTION );
	glFrustum( -x, x, -y, y, near, far );

	mat44 glMatrix; glGetFloatv( GL_PROJECTION_MATRIX, (float*)&glMatrix );
	printf( "glFrustum: %M\n", &glMatrix );

	mat44 myFrustum = mfrustum( -x, x, -y, y, near, far );
	printf( "myFrustum: %M\n", &myFrustum );


	float f = 1.f / tanf( .5f * deg2rad(fovy) );
	mat44 gluPerspective = {
		._1 = { f / aspect, 0.f, 0.f, 0.f },
		._2 = { 0.f, f, 0.f, 0.f },
		._3 = { 0.f, 0.f, (far + near) / (near - far), -1.f },
		._4 = { 0.f, 0.f, (2.f * far * near) / (near - far) }
	};

	printf( "gluPerspective: %M\n", &gluPerspective );

	return myFrustum;

}

mat44       ortho_View( float left, float right, float bottom, float top ) {

	return mortho( left, right, bottom, top, -1.f, 1.f );

}

View      compose_View( mat44 V, mat44 P ) {

	return (View) { V, P, mmul(V, P) };

}
