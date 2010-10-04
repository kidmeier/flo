#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "core.alloc.h"
#include "gl.array.h"
#include "gl.attrib.h"

// Vertex arrays //////////////////////////////////////////////////////////////

struct Varray {
	
	GLuint id;
	
	int        n;
	Vattrib**  attribs;

};

Varray* define_Varray(int n, ...) {
	
	Vattrib* vattribs[n];
	va_list args;
	
	va_start( args, n );
	for( int i=0; i<n; i++ )
		vattribs[i] = va_arg( args, Vattrib* );
	va_end(args);
	
	return new_Varray( n, vattribs );
}

Varray* new_Varray(int n, Vattrib* vattribs[]) {
	
	GLuint id;
	glGenVertexArrays( 1, &id );
	if( 0 == id )
		return NULL;
	
	Varray* va = new( NULL, Varray );
	
	// Setup the structure
	va->attribs = new_array( va, Vattrib*, n );
	if( !va->attribs ) {
		delete( va );
		return NULL;
	}
	
	// Setup the vertex array
	for( int i=0; i<n; i++ )
		va->attribs[i] = vattribs[i];
	
	return va;
}

void     delete_Varray( Varray* varray ) {
	
	assert( NULL != varray );
	
	glDeleteVertexArrays( 1, &varray->id );
	for( int i=0; i<varray->n; i++ )
		delete_Vattrib( varray->attribs[i] );
	
	memset( varray, 0, sizeof(Varray) );
	delete( varray );
	
}

Varray* bind_Varray( Varray* varray ) {
	
	// TODO: likely need some extra info in order to be able to obtain
	//       vertex attrib locations / bindings
	return NULL;
	
}
