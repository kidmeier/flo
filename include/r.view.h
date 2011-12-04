#ifndef __r_view_h__
#define __r_view_h__

#include "math.matrix.h"

typedef struct View View;

struct View {

	mat44 V;
	mat44 P;

	mat44 VP;

};

mat44 perspective_View( float fovy, float aspect, float near, float far );
mat44       ortho_View( float left, float right, float bottom, float top );

View      compose_View( mat44 V, mat44 P );

#endif
