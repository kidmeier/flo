#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "data.vector.h"

#define defaultCapacity 32

static void expand( Vector* vec ) {

	pointer newv = zalloc( vec->Z, 2 * vec->capacity * vec->it_size );
	memcpy( newv, vec->v, vec->size * vec->it_size );

	zfree( vec->Z, vec->v );
	vec->v = newv;
	vec->capacity = 2 * vec->capacity;
	
}


// Instantiation
Vector*   new_Vector( zone_p zone, uint it_size, int capacity ) {

	Vector* vec = zalloc( zone, sizeof(Vector) );

	vec->Z = zone;
	vec->it_size = it_size;
	vec->size = 0;
	vec->capacity = (capacity > 0 ? capacity : defaultCapacity);
	
	vec->v = zalloc( zone, vec->capacity * 	vec->it_size );

	return vec;

}

void      delete_Vector( Vector* v ) {

	zfree( v->Z, v->v );
	zfree( v->Z, v );

}

// Functions
uint    size_Vector( const Vector* v ) {
   
	return v->size;

}

uint    capacity_Vector( const Vector* v ) {

	return v->capacity;

}

int     index_of_Vector( const Vector* v, const pointer p ) {

	assert( 0 == ((uintptr_t)p % v->it_size) );

	if( p < v->v )
		return -1;

	uint extent = v->size * v->it_size;
	if( p + extent > v->v + extent )
		return -1;

	return (p - v->v) / v->it_size;

}

pointer first_Vector( const Vector* v ) {

	assert( !(isempty_Vector(v)) );
	return v->v;

}

pointer last_Vector( const Vector* v ) {

	assert( !(isempty_Vector(v)) );
	return v->v + (v->size-1) * v->it_size;

}

pointer nth_Vector( const Vector* v, int n ) {

	assert( n < v->size );
	return v->v + n * v->it_size;

}

pointer nth_from_last_Vector( const Vector* v, int n ) {

	assert( n < v->size );
	return v->v + (v->size - n - 1)*v->it_size;

}

pointer find_Vector( const Vector* v, bool (*predicate)(const pointer) ) {

	for( int i=0; i<size_Vector(v); i++ ) {

		const pointer it = nth_Vector(v, i);
		if( (*predicate)(it) )
			return it;

	}
	return NULL;

}

// Predicates
bool    isempty_Vector( const Vector* v ) {

	return 0 == v->size;

}

// Mutators
pointer pop_back_Vector( Vector* v ) {

	assert( !(isempty_Vector(v)) );
	return v->v + v->it_size * --v->size;
	
}

pointer push_back_Vector( Vector* v ) {

	if( v->size == v->capacity )
		expand(v);

	return v->v + v->it_size * v->size++;

}

#ifdef __data_vector_TEST__

#include <stdio.h>

#include "mm.heap.h"

int main( int argc, char* argv[] ) {

	const int N = 100;
	Vector* V = new_Vector( ZONE_heap, sizeof(int), N/2 );

	printf("size_Vector(V) = %d\n", size_Vector(V));
	printf("capacity_Vector(V) = %d\n", capacity_Vector(V));

	printf("push_back: ");
	for( int i=0; i<N; i++ ) {

		int* pi = push_back_Vector(V);
		(*pi) = i;

		printf("%d ", i);

	}
	printf("\n");

	printf("size_Vector(V) = %d\n", size_Vector(V));
	printf("capacity_Vector(V) = %d\n", capacity_Vector(V));

	for( int i=0; i<N; i++ ) {

		int* pi = nth_Vector(V, i);
		int* ri = nth_from_last_Vector(V, size_Vector(V) - i - 1);
		assert( (*pi) == i 
		        && (*ri) == i );

	}

	printf("\nOk\n");
	return 0;

}

#endif
