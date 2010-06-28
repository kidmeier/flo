#include <printf.h>
#include <stdio.h>
#include <stdlib.h>
#include "math.vec.h"

static int printf_FLOAT4(FILE* fp, const struct printf_info* info, const void* const *args) {

	const float4* V = *((const float4**)(args[0]));

	char* width,
		  * precision = "";
	asprintf(&width, "%d", info->left ? -info->width : info->width);
	if( info->prec >= 0 )
		asprintf(&precision, ".%d", info->prec);

	const char* space = info->space ? " " : "";
	const char* pad = info->pad == L'0' ? "0" : "";
	const char* showsign = info->showsign ? "+" : "";
	const char* group = info->group ? "'" : "";
	const char* quat = info->spec == L'Q' ? " | " : " ";

	// Build up our format string
	char* ffmt,
		  * vfmt;
	asprintf(&ffmt, "%%%s%s%s%s%s%sf", space, pad, group, showsign, width, precision);
	asprintf(&vfmt, "(%s %s %s%s%s)", ffmt, ffmt, ffmt, quat, ffmt);

	// Write to the stream
	int len = fprintf(fp, vfmt, V->x, V->y, V->z, V->w);

	if( info->prec >= 0 )
		free(precision);
	free(ffmt);
	free(vfmt);
	                   
	return len;
}

static int 
printf_arginfo_FLOAT4(const struct printf_info* info, size_t n, int* argtypes) {

	// We always take 1 arg and it is a pointer to the mat44
	if( n > 0 ) {
		argtypes[0] = PA_POINTER;
	}
	return 1;

}

// Constants
const float4 origin_PT = { 0.f, 0.f, 0.f, 1.f };
const float4 zero_VEC = { 0.f, 0.f, 0.f, 0.f };
const float4 xunit_VEC = { 1.f, 0.f, 0.f, 0.f };
const float4 yunit_VEC = { 0.f, 1.f, 0.f, 0.f };
const float4 zunit_VEC = { 0.f, 0.f, 1.f, 0.f };
const float4 wunit_VEC = { 0.f, 0.f, 0.f, 1.f };

// Printf
void init_printf_FLOAT4(void) {
	register_printf_function('V', printf_FLOAT4, printf_arginfo_FLOAT4);
	register_printf_function('Q', printf_FLOAT4, printf_arginfo_FLOAT4);
}

#ifdef __math_vec_TEST__

int main(int argc, char* argv[]) {

	init_printf_FLOAT4();

  float4 a = { 1.0f, 0.0f, 0.0f, 0.0f };
  float4 b = { 0.0f, 1.0f, 0.0f, 0.0f };
  float4 c = vadd(a, b);
  float4 q = { 1.0f, 0.0f, 0.0f, M_PI };

  printf("sizeof(float4) = %d\n", sizeof(float4));
  printf("a = %4.2V\n", &a);
  printf("b = %4.2V\n", &b);
  printf("c = %4.2V\n", &c);
  printf("q = %4.2Q\n", &q);

  return 0;

}

#endif
