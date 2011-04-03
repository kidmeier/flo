#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <GL/glew.h>

#include "core.alloc.h"
#include "gl.array.h"
#include "gl.attrib.h"

// Vertex arrays //////////////////////////////////////////////////////////////

struct Varray {
	
	GLuint id;

	Vindex  *index;
	
	int      n;
	Vattrib *attribs[];

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

Varray* new_Varray( int n, Vattrib *vattribs[] ) {

	return new_Varray_indexed( n, NULL, vattribs );

}

Varray *new_Varray_indexed( int n, Vindex *index, Vattrib *vattribs[] ) {
	
	GLuint id;
	glGenVertexArrays( 1, &id );
	if( 0 == id )
		return NULL;
	
	Varray* va = malloc( sizeof(Varray) + n*sizeof(Vattrib*) );
	if( !va )
		return NULL;

	// Setup the vertex array
	va->id = id; 
	va->n  = n;

	// Bind + enable vertex attribute arrays
	glBindVertexArray( id );
	for( int i=0; i<n; i++ ) {

		va->attribs[i] = vattribs[i];

		glEnableVertexAttribArray( i );
		glBindBuffer( GL_ARRAY_BUFFER, vattribs[i]->id );
		glVertexAttribPointer( i, 
		                       vattribs[i]->size, 
		                       vattribs[i]->type,
		                       vattribs[i]->normalize,
		                       vattribs[i]->stride,
		                       (GLvoid*)0 );

	}

	va->index = index;
	if( index )
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index->id );

	glBindVertexArray( 0 );
	return va;

}

void     delete_Varray( Varray* varray ) {
	
	assert( NULL != varray );
	
	glDeleteVertexArrays( 1, &varray->id );
	for( int i=0; i<varray->n; i++ )
		delete_Vattrib( varray->attribs[i] );
	
	memset( varray, 0, sizeof(Varray) );
	free( varray );
	
}

void draw_Varray( Varray* varray, GLenum mode, GLint first, GLsizei count ) {

	glBindVertexArray( varray->id );
	glDrawArrays( mode, first, count );
	
}

void draw_Varray_indexed( Vindex* index, Varray* varray, 
                          GLenum mode, 
                          GLsizeiptr first, 
                          GLsizei count ) {
	
	glBindVertexArray( varray->id );
	if( index != varray->index ) {
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index->id );
		varray->index = index;
	}

	glDrawElements( mode, count, GL_INT, (GLvoid*)first );

	glBindVertexArray( 0 );
	
}

