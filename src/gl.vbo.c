#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "core.alloc.h"
#include "gl.vbo.h"

// Buffers ////////////////////////////////////////////////////////////////////

GLuint new_BUF(void) {
  GLuint buf;
  glGenBuffers( 1, &buf );

  return buf;
}

void   delete_BUF( GLuint buf ) {
  glDeleteBuffers( 1, &buf );
}

int    alloc_BUF( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size ) {
  glBindBuffer( target, buf );
  glBufferData( target, size, NULL, usage );
  
  return 0;
}

void*  map_BUF( GLuint buf, GLenum target, GLenum access ) {

  glBindBuffer( target, buf);
  return glMapBuffer( target, access );

}

void*  map_range_BUF( GLuint buf, GLenum target, GLbitfield access, GLintptr ofs, GLsizeiptr len ) {
  glBindBuffer( target, buf );
  return glMapBufferRange( target, ofs, len, access );
}

void   flush_BUF( GLuint buf, GLenum target ) {
  glUnmapBuffer( target );
}

void   flush_range_BUF( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr len ) {
  glFlushMappedBufferRange( target, ofs, len );
}

int     upload_BUF( GLuint buf, GLenum target, GLenum usage, GLsizeiptr size, void* data ) {

  glBindBuffer( target, buf );
  glBufferData( target, size, data, usage );

  return 0;
}

int     upload_range_BUF( GLuint buf, GLenum target, GLintptr ofs, GLsizeiptr size, void* data ) {

  glBindBuffer( target, buf );
  glBufferSubData( target, ofs, size, data );

  return 0;
}

// Vertex buffers ////////////////////////////////////////////////////////////

struct vattrib_s {

  GLuint id;
  const char* name;

  uint arity;
  uint stride;
  float* buf;
};

vattrib_p new_VATTRIB( const char* name, uint arity, uint stride ) {

  GLuint id = new_BUF();
  if( 0 == id ) 
    return NULL;

  vattrib_p vattrib = new( NULL, vattrib_t );

  vattrib->id = id;
  vattrib->name = clone_string( vattrib, name );

  vattrib->arity = arity;
  vattrib->stride = stride;
  vattrib->buf = NULL;

  return vattrib;
}

void   delete_VATTRIB( vattrib_p vattrib ) {

  delete_BUF( vattrib->id );

  memset( vattrib, 0, sizeof(struct vattrib_s) );
  delete( vattrib );

}

int    alloc_VATTRIB( vattrib_p vattrib, uint size, GLenum usage ) {

  return alloc_BUF( vattrib->id, GL_ARRAY_BUFFER, usage, size );

}

float* map_VATTRIB( vattrib_p vattrib, GLenum access ) {

  assert( NULL == vattrib->buf );

  vattrib->buf = (float*)map_BUF( vattrib->id, GL_ARRAY_BUFFER, access );
  return vattrib->buf;
}

float* map_range_VATTRIB( vattrib_p vattrib, GLbitfield access, uint ofs, uint n ) {

  assert( NULL == vattrib->buf );
  vattrib->buf = (float*)map_range_BUF( vattrib->id,
					GL_ARRAY_BUFFER,
					access,
					ofs * vattrib->stride,
					n * vattrib->stride );
  
  return vattrib->buf;
}

void   flush_VATTRIB( vattrib_p vattrib ) {

  assert( NULL != vattrib->buf );
  
  flush_BUF( vattrib->id, GL_ARRAY_BUFFER );
  vattrib->buf = NULL;

}

void   flush_range_VATTRIB( vattrib_p vattrib, uint ofs, uint n ) {

  assert( NULL != vattrib->buf );

  flush_range_BUF( vattrib->id, GL_ARRAY_BUFFER, 
		   ofs * vattrib->stride, 
		   n * vattrib->stride );
  vattrib->buf = NULL;
}

int    upload_VATTRIB( vattrib_p vattrib, GLenum usage, uint n, float* data ) {

  return upload_BUF( vattrib->id, GL_ARRAY_BUFFER, usage, n * vattrib->stride, data );

}

int    upload_range_VATTRIB( vattrib_p vattrib, uint ofs, uint size, float* data ) {

  return upload_range_BUF( vattrib->id, GL_ARRAY_BUFFER, ofs, size, data );

}

// Index buffers //////////////////////////////////////////////////////////////

struct vindex_s {

  GLuint id;
  int*   buf;

};

vindex_p  new_VINDEX( void ) {

  GLuint id = new_BUF();
  if( 0 == id ) 
    return NULL;

  vindex_p vindex = new( NULL, vindex_t );
  
  vindex->id = id;
  vindex->buf = NULL;

  return vindex;
}

void    delete_VINDEX( vindex_p vindex ) {
  
  delete_BUF( vindex->id );

  memset( vindex, 0, sizeof(vindex_t) );
  delete( vindex );

}

int     upload_VINDEX( vindex_p vindex, GLenum usage, uint n, int* data ) {

  return upload_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, usage, n * sizeof(uint), data );

}

int     upload_range_VINDEX( vindex_p vindex, uint ofs, uint n, int* data ) {

  return upload_range_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, 
			   ofs * sizeof(uint), 
			   n * sizeof(uint),
			   data );
}

int     alloc_VINDEX( vindex_p vindex, GLenum usage, uint n  ) {

  return alloc_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, usage, n * sizeof(uint) );

}

int*    map_VINDEX( vindex_p vindex, GLenum access ) {

  assert( NULL == vindex->buf );

  vindex->buf = (int*)map_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, access );
  return vindex->buf;  

}

int*    open_range_VINDEX( vindex_p vindex, uint ofs, uint n, GLenum rw, GLbitfield access ) {

  assert( NULL == vindex->buf );

  vindex->buf = (int*)map_range_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER,
				     access,
				     ofs * sizeof(uint),
				     n * sizeof(uint) );
  
  return vindex->buf;
}

void    flush_VINDEX( vindex_p vindex ) {

  assert( NULL != vindex->buf );

  flush_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER );
  vindex->buf = NULL;
}

void    flush_range_VINDEX( vindex_p vindex, uint ofs, uint n ) {

  assert( NULL != vindex->buf );

  flush_range_BUF( vindex->id, GL_ELEMENT_ARRAY_BUFFER, 
		   ofs * sizeof(uint),
		   n * sizeof(uint) );
}

// Vertex arrays //////////////////////////////////////////////////////////////

struct varray_s {

  GLuint id;

  int        n;
  vattrib_p* attribs;
};

varray_p define_VARRAY(int n, ...) {

  vattrib_p vattribs[n];
  va_list args;

  va_start( args, n );
  for( int i=0; i<n; i++ )
    vattribs[i] = va_arg( args, vattrib_p );
  va_end(args);

  return new_VARRAY( n, vattribs );
}

varray_p new_VARRAY(int n, vattrib_p vattribs[]) {

  GLuint id;
  glGenVertexArrays( 1, &id );
  if( 0 == id )
    return NULL;

  varray_p va = new( NULL, varray_t );

  // Setup the structure
  va->attribs = new_array( va, vattrib_p, n );
  if( !va->attribs ) {
    delete( va );
    return NULL;
  }

  // Setup the vertex array
  for( int i=0; i<n; i++ )
    va->attribs[i] = vattribs[i];

  return va;
}

void     delete_VARRAY( varray_p varray ) {

  assert( NULL != varray );

  glDeleteVertexArrays( 1, &varray->id );
  for( int i=0; i<varray->n; i++ )
    delete_VATTRIB( varray->attribs[i] );

  memset( varray, 0, sizeof(varray_t) );
  delete( varray );

}

varray_p bind_VARRAY( varray_p varray ) {

  // TODO: likely need some extra info in order to be able to obtain
  //       vertex attrib locations / bindings
  return NULL;

}
