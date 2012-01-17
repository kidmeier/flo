#ifndef __math_util_h__
#define __math_util_h__

#include "control.minmax.h"
#include "core.types.h"

static inline uint16 log2u( uint x ) {

	uint16 y = 0;
	while( x ) {
		y++;
		x >>= 2;
	}
	return y;

}

static inline uint ceilu( uint x, uint mod ) {

	return x + (mod - (x % mod));

}

static inline uint ceil2u( uint x ) {

	uint16 mod = 1U << log2u(x);
	return min(x, ceilu(x, (mod << 1U)));

}

static inline uint flooru( uint x, uint mod ) {

	return x - (x % mod);

}

static inline float deg2rad( float x ) {

	return ( M_PI * x / 180.f );

}

static inline float rad2deg( float x ) {

	return ( 180.f * x / M_PI );

}

#endif
