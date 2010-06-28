#include <printf.h>
#include <stdio.h>
#include <stdlib.h>
#include "math.matrix.h"

static int printf_MAT44(FILE* fp, const struct printf_info* info, const void* const *args) {

	const mat44* M = *((const mat44**)(args[0]));
	int len = 0;

	char* width,
		  * precision = "";
	asprintf(&width, "%d", info->left ? -info->width : info->width);
	if( info->prec >= 0 )
		asprintf(&precision, ".%d", info->prec);

	const char* space = info->space ? " " : "";
	const char* pad = info->pad == L'0' ? "0" : "";
	const char* showsign = info->showsign ? "+" : "";
	const char* group = info->group ? "'" : "";

	// Print on its own 4 lines
	if( info->alt ) {

		char* ffmt,
			  * mfmt;
		len = asprintf(&ffmt, "%%%s%s%s%s%s%sf", space, pad, group, showsign, width, precision);
		if( len < 0 ) return len;
		len = asprintf(&mfmt, "\n| %s %s %s %s |\n| %s %s %s %s |\n| %s %s %s %s |\n| %s %s %s %s |\n",
		         ffmt, ffmt, ffmt, ffmt, ffmt, ffmt, ffmt, ffmt, 
		         ffmt, ffmt, ffmt, ffmt, ffmt, ffmt, ffmt, ffmt);
		if( len < 0 ) {
			free(ffmt);
			return len;
		}

		len = fprintf(fp, mfmt, 
		              rowcol(M,1,1), rowcol(M,1,2), rowcol(M,1,3), rowcol(M,1,4),
		              rowcol(M,2,1), rowcol(M,2,2), rowcol(M,2,3), rowcol(M,2,4),
		              rowcol(M,3,1), rowcol(M,3,2), rowcol(M,3,3), rowcol(M,3,4),
		              rowcol(M,4,1), rowcol(M,4,2), rowcol(M,4,3), rowcol(M,4,4));

		free(ffmt);
		free(mfmt);

		// Print column vectors all in one line
	} else {
		// Build up our format string
		char* vfmt,
			  * mfmt;
		len = asprintf(&vfmt, "%%%s%s%s%s%s%sV", space, pad, group, showsign, width, precision);
		if( len < 0 ) return len;
		len = asprintf(&mfmt, "| X=%s, Y=%s, Z=%s, W=%s |", vfmt, vfmt, vfmt, vfmt);
		if( len < 0 ) {
			free(vfmt);
			return len;
		}

		// Write to the stream
		len = fprintf(fp, mfmt, &M->_1, &M->_2, &M->_3, &M->_4);
		free(vfmt);
		free(mfmt);
	}

	if( info->prec >= 0 )
		free(precision);
	
	return len;
}

static int 
printf_arginfo_MAT44(const struct printf_info* info, size_t n, int* argtypes) {

	// We always take 1 arg and it is a pointer to the mat44
	if( n > 0 ) {
		argtypes[0] = PA_POINTER;
	}
	return 1;

}

// Constants
const mat44 identity_MAT44 = {
  ._1 = { 1.f, 0.f, 0.f, 0.f },
  ._2 = { 0.f, 1.f, 0.f, 0.f },
  ._3 = { 0.f, 0.f, 1.f, 0.f },
  ._4 = { 0.f, 0.f, 0.f, 1.f }
};

const mat44 zero_MAT44 = {
  ._1 = { 0.f, 0.f, 0.f, 0.f },
  ._2 = { 0.f, 0.f, 0.f, 0.f },
  ._3 = { 0.f, 0.f, 0.f, 0.f },
  ._4 = { 0.f, 0.f, 0.f, 0.f }
};

// Printf
void init_printf_MAT44(void) {
	init_printf_FLOAT4();
	register_printf_function('M', printf_MAT44, printf_arginfo_MAT44);
}

#ifdef __math_matrix_TEST__

int main(int argc, char* argv[]) {
	
	init_printf_MAT44();

	const float pi2 = M_PI / 2.f;
	const float pi4 = M_PI / 4.f;
	const float pi6 = M_PI / 6.f;

	const float4 qI = qaxis((float4){ 1.f, 0.f, 0.f, 0.f });
	const float4 qX = qaxis((float4){ 1.f, 0.f, 0.f, pi6 });
	const float4 qY = qaxis((float4){ 0.f, 1.f, 0.f, pi6 });
	const float4 qZ = qaxis((float4){ 0.f, 0.f, 1.f, pi6 });

	const float4 X = axisq(qX);
	const float4 Y = axisq(qY);
	const float4 Z = axisq(qZ);

	const mat44 xrot = mrotateX(pi6);
	const mat44 yrot = mrotateY(pi6);
	const mat44 zrot = mrotateZ(pi6);
	const mat44 qIrot = qmatrix( qI );
	const mat44 qxrot = qmatrix( qX );
	const mat44 qyrot = qmatrix( qY );
	const mat44 qzrot = qmatrix( qZ );
	
	const mat44 xyzrot = mmul(mmul(xrot,yrot),zrot);
	const mat44 qxyzrot = qmatrix( qmul(qmul(qX,qY),qZ) );

	const float4 p = { 1.f, 0.f, 0.f, 1.f };
	const float4 pp = mtransform(zrot, p);
	const float4 qpp = qrot(qZ, p);

	printf("I = % #4.1M\n", &identity_MAT44);
	printf("qI = % #4.1M\n", &qIrot);

	printf("rotationX(pi / 6) = % #4.2M\n", &xrot);
	printf("qmatrix(qaxis(X, pi / 6)) = % #4.2M\n", &qxrot);
	printf("rotationY(pi / 6) = % #4.2M\n", &yrot);
	printf("qmatrix(qaxis(Y, pi / 6)) = % #4.2M\n", &qyrot);
	printf("rotationZ(pi / 6) = % #4.2M\n", &zrot);
	printf("qmatrix(qaxis(Z, pi / 6)) = % #4.2M\n", &qzrot);

	printf("rotationX(pi / 6)*rotationY(pi /6)*rotationZ(pi / 6) = % #4.2M\n", &xyzrot);
	printf("qmatrix( qX*qY*qZ ) = % #4.2M\n", &qxyzrot);

	printf("p = % 4.2V\n", &p);
	printf("rotationZ(pi / 6) * p = % 4.2V\n", &pp);
	printf("qrot(qZ, p) = % 4.2V\n", &qpp);

	printf("\n");

	printf("q% 4.2V ~ aa% 4.2V\n", &qX, &X);
	printf("q% 4.2V ~ aa% 4.2V\n", &qY, &Y);
	printf("q% 4.2V ~ aa% 4.2V\n", &qZ, &Z);

	return 0;
}


#endif
