#ifndef __gl_geom_h__
#define __gl_geom_h__

#include <GL/glew.h>
#include "core.types.h"

/*
typedef enum {

  gl_Vertex = 0,
  gl_Color,
  gl_SecondaryColor,
  gl_Normal,
  gl_FogCoord,
  gl_VertexID,
  gl_InstanceID,
  gl_TexCoord0,
  gl_Attrib0 = 0x1000000,

} vattrib_e;
*/

// Raw buffers ////////////////////////////////////////////////////////////////
GLuint new_BUF(void);
void   delete_BUF( GLuint buf );

int    alloc_BUF( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size );
void*  map_BUF( GLuint buf, GLenum target, GLenum access );
void*  map_range_BUF( GLuint buf, GLenum target, GLbitfield access, GLintptr ofs, GLsizeiptr len );
void   flush_BUF( GLuint /*buf*/, GLenum target );
void   flush_range_BUF( GLuint /*buf*/, GLenum target, GLintptr ofs, GLsizeiptr len );

int    upload_BUF( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size, void* data );
int    upload_range_BUF( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr size, void* data );

// Vertex attribute buffers ///////////////////////////////////////////////////
struct vattrib_s;
typedef struct vattrib_s vattrib_t;
typedef vattrib_t* vattrib_p;

vattrib_p new_VATTRIB( const char* name, uint arity, uint stride );
void      delete_VATTRIB( vattrib_p vattrib );

int    alloc_VATTRIB( vattrib_p vattrib, uint n, GLenum usage );
float* map_VATTRIB( vattrib_p vattrib, GLenum access );
float* map_range_VATTRIB( vattrib_p vattrib, GLbitfield access, uint ofs, uint n );
void   flush_VATTRIB( vattrib_p vattrib );
void   flush_range_VATTRIB( vattrib_p vattrib, uint ofs, uint n );

int    upload_VATTRIB( vattrib_p vattrib, GLenum usage, uint n, float* data );
int    upload_range_VATTRIB( vattrib_p vattrib, uint ofs, uint n, float* data );

// Vertex arrays ////////////////////////////////////////////////////////////

struct varray_s;
typedef struct varray_s varray_t;
typedef varray_t* varray_p;

//varray_p define_VARRAY(int n, ...);
varray_p new_VARRAY(int n, vattrib_p vattribs[]);
void     delete_VARRAY( varray_p varray );
varray_p bind_VARRAY( varray_p varray );

// Index buffers //////////////////////////////////////////////////////////////

struct vindex_s;
typedef struct vindex_s vindex_t;
typedef vindex_t* vindex_p;

vindex_p  new_VINDEX( void );
void      delete_VINDEX( vindex_p vindex );

int     upload_VINDEX( vindex_p vindex, GLenum usage, uint n, int* data );
int     upload_range_VINDEX( vindex_p vindex, uint ofs, uint n, int* data );

int     alloc_VINDEX( vindex_p vindex, uint n, GLenum usage );
int*    open_VINDEX( vindex_p vindex, GLenum access );
int*    open_range_VINDEX( vindex_p vindex, uint ofs, uint n, GLenum rw, GLbitfield access );
void    flush_VINDEX( vindex_p vindex );
void    flush_range_VINDEX( vindex_p vindex, uint ofs, uint N );

#endif
