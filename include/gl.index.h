#ifndef __gl_index_h__
#define __gl_index_h__

#include "gl.buf.h"

// Index buffers //////////////////////////////////////////////////////////////

typedef struct vindex_s  vindex_t;
typedef struct vindex_s* vindex_p;

vindex_p  new_VINDEX( void );
void      delete_VINDEX( vindex_p vindex );

int     upload_VINDEX( vindex_p vindex, GLenum usage, uint n, int* data );
int     upload_range_VINDEX( vindex_p vindex, uint ofs, uint n, int* data );

int*    alloc_VINDEX( vindex_p vindex, uint n, GLenum usage );
int*    map_VINDEX( vindex_p vindex, GLenum access );
int*    map_range_VINDEX( vindex_p vindex, uint ofs, uint n, GLenum rw, GLbitfield access );
void    flush_VINDEX( vindex_p vindex );
void    flush_range_VINDEX( vindex_p vindex, uint ofs, uint N );

#endif
