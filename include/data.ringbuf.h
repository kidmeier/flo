#ifndef __data_ringbuf_h__
#define __data_ringbuf_h__

#include "core.types.h"

typedef struct ringbuf_s ringbuf_t;
typedef ringbuf_t* ringbuf_p;

ringbuf_p new_RINGBUF( uint16 size, uint16 count );
void      destroy_RINGBUF( ringbuf_p buf );

// Writes `size` bytes to `buf` from `data`. Returns `size` on success,
// or the negated maximum number of bytes that can be written (e.g. -4 means 
// a maximum of 4 bytes can be written until some bytes are read)
int       write_RINGBUF( ringbuf_p buf, uint16 size, pointer data );

// Reads `size` bytes from `buf` to `data`. Returns `size` on success,
// or the negated maximum number of bytes that can be read (e.g. -4 means 
// a maximum of 4 bytes can be read until some more bytes are written)
int       read_RINGBUF( ringbuf_p buf, uint16 size, pointer dest );

#endif
