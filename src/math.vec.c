#include "core.features.h"

#if defined( feature_GLIBC )
#include <printf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include "control.maybe.h"
#include "math.vec.h"
#include "math.util.h"

#if defined( feature_GLIBC )

static int printf_FLOAT4(FILE* fp, const struct printf_info* info, const void* const *args) {

	const float4* V = *((const float4**)(args[0]));

	char* width = NULL,
		  * precision = "";
	int ret = asprintf(&width, "%d", info->left ? -info->width : info->width);

	if( info->prec >= 0 ) {
		ret = maybe( ret, < 0, asprintf(&precision, ".%d", info->prec) );
	}

	const char* space = info->space ? " " : "";
	const char* pad = info->pad == L'0' ? "0" : "";
	const char* showsign = info->showsign ? "+" : "";
	const char* group = info->group ? "'" : "";
	const char* quat = info->spec == L'Q' ? " | " : " ";

	// Build up our format string
	char* ffmt = NULL,
		  * vfmt = NULL;
	ret = maybe( ret, < 0, 
	             asprintf(&ffmt, "%%%s%s%s%s%s%sf", space, pad, group, showsign, width, precision) 
		);
	ret = maybe( ret, < 0,
	             asprintf(&vfmt, "(%s %s %s%s%s)", ffmt, ffmt, ffmt, quat, ffmt)
		);

	// Write to the stream
	if( ret >= 0 )
		ret = fprintf(fp, vfmt, V->x, V->y, V->z, V->w);

	if( info->prec >= 0 ) free(precision);
	if( width ) free(width);
	if( ffmt ) free(ffmt);
	if( vfmt ) free(vfmt);
	
	return ret;
}

static int 
printf_arginfo_FLOAT4(const struct printf_info* info, size_t n, int* argtypes) {

	// We always take 1 arg and it is a pointer to the mat44
	if( n > 0 ) {
		argtypes[0] = PA_POINTER;
	}
	return 1;

}

#endif // defined( feature_GLIBC )

// Constants
const float4 origin_PT = { 0.f, 0.f, 0.f, 1.f };
const float4 zero_VEC = { 0.f, 0.f, 0.f, 0.f };
const float4 xunit_VEC = { 1.f, 0.f, 0.f, 0.f };
const float4 yunit_VEC = { 0.f, 1.f, 0.f, 0.f };
const float4 zunit_VEC = { 0.f, 0.f, 1.f, 0.f };
const float4 wunit_VEC = { 0.f, 0.f, 0.f, 1.f };

// Printf
void init_printf_FLOAT4(void) {
#if defined( feature_GLIBC )
	register_printf_function('V', printf_FLOAT4, printf_arginfo_FLOAT4);
	register_printf_function('Q', printf_FLOAT4, printf_arginfo_FLOAT4);
#endif
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
