#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "core.alloc.h"
#include "gl.buf.h"
#include "gl.attrib.h"

// Attribute buffers //////////////////////////////////////////////////////////

struct Vattrib {
	
	GLuint      id;
	const char* name;
	
	uint        stride;
	pointer     buf;
	
};

Vattrib*  new_Vattrib( const char* name, uint width ) {
	
	GLuint id = new_Buf();
	if( 0 == id ) 
		return NULL;
	
	Vattrib* vattrib = new( NULL, Vattrib );
	
	vattrib->id = id;
	vattrib->name = clone_string( vattrib, name );
	
	vattrib->stride = width;
	vattrib->buf = NULL;
	
	return vattrib;
}

void   delete_Vattrib( Vattrib* vattrib ) {
	
	delete_Buf( vattrib->id );
	
	memset( vattrib, 0, sizeof(struct Vattrib) );
	delete( vattrib );
	
}

pointer alloc_Vattrib( Vattrib* vattrib, uint size, GLenum usage ) {
	
	return alloc_Buf( vattrib->id, GL_ARRAY_BUFFER, usage, size );
	
}

pointer   map_Vattrib( Vattrib* vattrib, GLenum access ) {
	
	assert( NULL == vattrib->buf );
	
	vattrib->buf = map_Buf( vattrib->id, GL_ARRAY_BUFFER, access );
	return vattrib->buf;

}

pointer   map_Vattrib_range( Vattrib* vattrib, GLbitfield access, uint ofs, uint n ) {
	
	assert( NULL == vattrib->buf );
	vattrib->buf = map_Buf_range( vattrib->id,
	                              GL_ARRAY_BUFFER,
	                              access,
	                              ofs * vattrib->stride,
	                              n * vattrib->stride );
	
	return vattrib->buf;

}

void    flush_Vattrib( Vattrib* vattrib ) {
	
	assert( NULL != vattrib->buf );
	
	flush_Buf( vattrib->id, GL_ARRAY_BUFFER );
	vattrib->buf = NULL;
	
}

void    flush_Vattrib_range( Vattrib* vattrib, uint ofs, uint n ) {
	
	assert( NULL != vattrib->buf );
	
	flush_Buf_range( vattrib->id, GL_ARRAY_BUFFER, 
	                 ofs * vattrib->stride, 
	                 n * vattrib->stride );
	vattrib->buf = NULL;

}

int    upload_Vattrib( Vattrib* vattrib, GLenum usage, uint n, pointer data ) {
	
	return upload_Buf( vattrib->id, GL_ARRAY_BUFFER, usage, n * vattrib->stride, data );
	
}

int    upload_Vattrib_range( Vattrib* vattrib, uint ofs, uint n, pointer data ) {
	
	return upload_Buf_range( vattrib->id, GL_ARRAY_BUFFER, 
	                         ofs * vattrib->stride, 
	                         n * vattrib->stride, 
	                         data );
	
}
