#ifndef __data_bitset_h__
#define __data_bitset_h__

#include <string.h>
#include "core.types.h"

#define bitset_uint32_count(bits) \
	(1 + ((bits)-1)/32)

#define bitset_uint32_index(bit)	  \
	((bit) / 32)

#define bitset( name, bits )	  \
	uint32 name[ bitset_uint32_count( (bits) ) ]

static inline void bitset_clearall( uint32* bitset, int nbits ) {
	
	int size = bitset_uint32_count(nbits);

	for( int i=0; i<size; i++ )
		bitset[i] = 0;
	
}

static inline bool bitset_isset( const uint32* bitset, int bit ) {

	int index = bitset_uint32_index( bit );
	return 0 != (bitset[index] & (1 << (bit - 32*index)) );

}

static inline void bitset_set( uint32* bitset, int bit ) {

	int index = bitset_uint32_index( bit );
	bitset[index] |= (1 << (bit - 32*index));

}

static inline void bitset_clear( uint32* bitset, int bit ) {

	int index = bitset_uint32_index( bit );
	bitset[index] &= ~(1 << (bit - 32*index));

}

static inline void bitset_copy( uint32* dest, int nbits, const uint32* src ) {

	int size = bitset_uint32_count(nbits);
	memcpy( dest, src, size * sizeof(uint32) );

}

#endif
