#ifndef __gl_index_h__
#define __gl_index_h__

#include "gl.buf.h"

// Index buffers //////////////////////////////////////////////////////////////

typedef struct Vindex  Vindex;

Vindex* new_Vindex( void );
void delete_Vindex( Vindex* vindex );

int  upload_Vindex( Vindex* vindex, GLenum usage, uint n, int* data );
int  upload_Vindex_range( Vindex* vindex, uint ofs, uint n, int* data );

int*  alloc_Vindex( Vindex* vindex, uint n, GLenum usage );
int*    map_Vindex( Vindex* vindex, GLenum access );
int*    map_Vindex_range( Vindex* vindex, uint ofs, uint n, GLenum rw, GLbitfield access );
void  flush_Vindex( Vindex* vindex );
void  flush_Vindex_range( Vindex* vindex, uint ofs, uint N );

#endif
