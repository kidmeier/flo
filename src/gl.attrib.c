#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "gl.attrib.h"
#include "gl.buf.h"
#include "gl.types.h"

// Attribute buffers //////////////////////////////////////////////////////////

Vattrib*  new_Vattrib( const char* name, 
                       short     size, 
                       GLenum    type, 
                       GLboolean normalize ) {
	
	GLuint id = new_Buf();
	if( 0 == id ) 
		return NULL;
	
	Vattrib* vattrib = malloc( sizeof(Vattrib) + strlen(name)+1 );
	
	vattrib->id = id;

	strcpy( (char*)( vattrib + 1 ), name );
	vattrib->name = (char*)( vattrib + 1 );

	vattrib->size      = size;
	vattrib->type      = type;
	vattrib->normalize = normalize;
	vattrib->stride    = size * sizeof_GLtype( type );
	vattrib->buf       = NULL;
	
	return vattrib;
}

void   delete_Vattrib( Vattrib* vattrib ) {
	
	delete_Buf( vattrib->id );	
	free( vattrib );
	
}

pointer alloc_Vattrib( Vattrib* vattrib, GLenum usage, GLsizeiptr count ) {
	
	vattrib->buf = alloc_Buf( vattrib->id, GL_ARRAY_BUFFER, 
	                          usage,
	                          vattrib->stride * count );
	return vattrib->buf;

}

pointer   map_Vattrib( Vattrib* vattrib, GLenum access ) {
	
	assert( NULL == vattrib->buf );
	
	vattrib->buf = map_Buf( vattrib->id, GL_ARRAY_BUFFER, access );
	return vattrib->buf;

}

pointer   map_Vattrib_range( Vattrib* vattrib, GLbitfield access, GLintptr ofs, GLsizeiptr n ) {
	
	assert( NULL == vattrib->buf );
	vattrib->buf = map_Buf_range( vattrib->id,
	                              GL_ARRAY_BUFFER,
	                              access,
	                              ofs * vattrib->stride,
	                              n   * vattrib->stride );
	
	return vattrib->buf;

}

void    flush_Vattrib( Vattrib* vattrib ) {
	
	assert( NULL != vattrib->buf );
	
	flush_Buf( vattrib->id, GL_ARRAY_BUFFER );
	vattrib->buf = NULL;
	
}

void    flush_Vattrib_range( Vattrib* vattrib, GLintptr ofs, GLsizeiptr n ) {
	
	assert( NULL != vattrib->buf );
	
	flush_Buf_range( vattrib->id, GL_ARRAY_BUFFER, 
	                 ofs * vattrib->stride, 
	                 n   * vattrib->stride );
	vattrib->buf = NULL;

}

int    upload_Vattrib( Vattrib* vattrib, GLenum usage, GLsizeiptr n, pointer data ) {
	
	return upload_Buf( vattrib->id, GL_ARRAY_BUFFER, 
	                   usage, 
	                   n * vattrib->stride, 
	                   data );
	
}

int    upload_Vattrib_range( Vattrib* vattrib, GLintptr ofs, GLsizeiptr n, pointer data ) {
	
	return upload_Buf_range( vattrib->id, GL_ARRAY_BUFFER, 
	                         ofs * vattrib->stride, 
	                         n   * vattrib->stride, 
	                         data );
	
}
