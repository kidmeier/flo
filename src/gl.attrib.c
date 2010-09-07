#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "core.alloc.h"
#include "gl.buf.h"
#include "gl.attrib.h"

// Attribute buffers //////////////////////////////////////////////////////////

struct vattrib_s {
	
	GLuint id;
	const char* name;
	
	uint arity;
	uint mb_sz;

	uint stride;
	float* buf;
	
};

vattrib_p new_VATTRIB( const char* name, uint arity, uint mb_sz ) {
	
	GLuint id = new_BUF();
	if( 0 == id ) 
		return NULL;
	
	vattrib_p vattrib = new( NULL, vattrib_t );
	
	vattrib->id = id;
	vattrib->name = clone_string( vattrib, name );
	
	vattrib->arity = arity;
	vattrib->mb_sz = mb_sz;

	vattrib->stride = arity * mb_sz;
	vattrib->buf = NULL;
	
	return vattrib;
}

void   delete_VATTRIB( vattrib_p vattrib ) {
	
	delete_BUF( vattrib->id );
	
	memset( vattrib, 0, sizeof(struct vattrib_s) );
	delete( vattrib );
	
}

float* alloc_VATTRIB( vattrib_p vattrib, uint size, GLenum usage ) {
	
	return (float*)alloc_BUF( vattrib->id, GL_ARRAY_BUFFER, usage, size );
	
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

int    upload_range_VATTRIB( vattrib_p vattrib, uint ofs, uint n, float* data ) {
	
	return upload_range_BUF( vattrib->id, GL_ARRAY_BUFFER, 
	                         ofs * vattrib->stride, 
	                         n * vattrib->stride, 
	                         data );
	
}
