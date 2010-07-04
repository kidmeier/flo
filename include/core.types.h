#ifndef __core_types_H__
#define __core_types_H__

#include <sys/types.h>

// Sized integer types
typedef int8_t    int8;
typedef u_int8_t  uint8;
typedef int16_t   int16;
typedef u_int16_t uint16;
typedef int32_t   int32;
typedef u_int32_t uint32;
typedef int64_t   int64;
typedef u_int64_t uint64;

// Convenient aliases
typedef unsigned int   uint;
typedef unsigned char  uchar;
typedef unsigned char  byte;
typedef unsigned short ushort;
typedef unsigned long  ulong;

// Pointer type aliases
typedef void*     any;
typedef char*     string;

// Bool
#include <stdbool.h>

#define ofs_of(typ, field) \
	( (int)&((typ *)0) -> field )

#define field_ofs(addr, ofs, cast)	  \
	(cast *)( (char*)addr + ofs )

#endif
