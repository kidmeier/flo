#ifndef __data_vector_h__
#define __data_vector_h__

#include "core.types.h"
#include "mm.zone.h"

typedef struct Vector Vector;
struct Vector {

	zone_p     Z;
	uint       it_size;

	int        size;
	int        capacity;

	pointer    v;

};

// Dynamically expandable arrays. Constructed with an item size and initial
// capacity, the array is dynamically reallocated if an attempt is made to
// add an item beyond the current capacity.
//
// Storage is guaranteed to be allocated in a contiguous chunk of memory.
//
// Caution: The resizing process performs a full copy of existing data into the
//          expanded array. This means it is expensive, and second that all
//          current pointers to elements in the array become invalid.

// Instantiation
Vector*   new_Vector( zone_p zone, uint li_size, int capacity );
void      delete_Vector( Vector* v );

// Functions
uint    size_Vector( const Vector* v );
uint    capacity_Vector( const Vector* v );

uint    index_of_Vector( const Vector* v, const pointer p );

pointer first_Vector( const Vector* v );
pointer last_Vector( const Vector* v );
pointer nth_Vector( const Vector* v, int n );
pointer nth_from_last_Vector( const Vector* v, int n );

pointer find_Vector( const Vector* v, bool (*predicate)(const pointer) );

// Predicates
bool    isempty_Vector( const Vector* v );

// Mutators
pointer pop_back_Vector( Vector* v );
pointer push_back_Vector( Vector* v );

// Macros
#define find__Vector( v, node, pred )	  \
	do { \
		for( int i=0; i<size_Vector( (v) ); i++ ) { \
			(node) = nth_Vector( (v), i ); \
			if( pred ) { \
				break; \
			} \
		} \
	} while( 0 )

#endif
