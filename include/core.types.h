#ifndef __core_types_H__
#define __core_types_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Sized integer types
typedef int8_t   int8;
typedef uint8_t  uint8;
typedef int16_t  int16;
typedef uint16_t uint16;
typedef int32_t  int32;
typedef uint32_t uint32;
typedef int64_t  int64;
typedef uint64_t uint64;

// Convenient aliases
typedef unsigned int   uint;
typedef unsigned char  uchar;
typedef unsigned char  byte;
typedef unsigned short ushort;
typedef unsigned long  ulong;

// Pointer type aliases
typedef void* pointer;

// Common ptr-to-function prototypes
typedef bool (*predicate_f)( pointer );

#define field_ofs(addr, ofs, cast)	  \
	(cast *)( (void*)addr + ofs )

#endif
