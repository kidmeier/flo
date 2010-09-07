#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "core.alloc.h"
#include "gl.array.h"
#include "gl.attrib.h"

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
