#include "mm.stack.h"

struct Stack {

	zone_p  Z;

	pointer base;
	pointer sp;

	pointer limit;

};

// Instantiations
Stack*  new_Stack( zone_p Z, int sz ) {

	Stack* s = zalloc(Z, sizeof(Stack));

	s->Z     = Z;
	s->base  = zalloc( Z, sz );
	s->sp    = s->base;
	s->limit = s->base + sz;

	return s;

}

void    delete_Stack( Stack* stack ) {

	zfree( stack->Z, stack->base );
	zfree( stack->Z, stack );

}

// Functions
pointer tell_Stack( const Stack* stack ) {

	return stack->sp;

}

size_t  capacity_Stack( const Stack* stack ) {

	return stack->limit - stack->base;

}

size_t  remaining_Stack( const Stack* stack ) {

	return stack->limit - stack->sp;

}

// Mutators
pointer push_Stack( Stack* stack, int sz ) {

	if( stack->sp + sz <= stack->limit ) {

		pointer p = stack->sp;
		stack->sp = stack->sp + sz;

		return p;
		
	}

	return NULL;

}

pointer dup_Stack( Stack* stack, int sz, pointer data ) {

	if( stack->sp + sz <= stack->limit ) {

		pointer p = stack->sp;
		stack->sp = stack->sp + sz;

		memcpy( p, data, sz );
		return p;

	}

	return NULL;

}

pointer pop_Stack( Stack* stack, int sz ) {

	if( stack->sp - sz >= stack->base ) {

		stack->sp = stack->sp - sz;
		return stack->sp;

	}

	return NULL;

}

pointer grow_Stack( Stack* stack, int new_sz ) {

	pointer newbase = zrealloc( stack->Z, stack->base, capacity_Stack(stack), new_sz );
	
	if( newbase ) {

		size_t sp = stack->sp - stack->base;

		stack->base  = newbase;
		stack->sp    = stack->base + sp;
		stack->limit = stack->base + new_sz;

	}
	return stack->sp;

}

#ifdef __mm_stack_TEST__

#include <assert.h>
#include <stdio.h>
#include "mm.heap.h"

int main( int argc, char* argv[] ) {

	Stack*   s = new_Stack( ZONE_heap, 32 * sizeof(int) );

	assert( s->sp == s->base );

	int count = 0;
	while( 1 ) {

		int* sp = (int*)push_Stack(s, sizeof(int));
		if( !sp )
			break;

		(*sp) = count;
		count++;

	}

	assert( s->sp == s->limit );

	printf("Pushed %d elements to the stack:\n", count);
	for( int i=0; i<count; i++ ) {

		int* n = pop_Stack( s, sizeof(int) );
		printf("\t%d\n", (*n));

	}

	assert( s->sp == s->base );

	for( int i=0; i<count; i++ ) {

		int n = count - i;
		dup_Stack( s, sizeof(int), &n );

	}

	assert( s->sp == s->limit );

	printf("Dupped %d elements to the stack:\n", count);
	while( 1 ) {

		int* n = pop_Stack( s, sizeof(int) );
		if( !n )
			break;

		printf("\t%d\n", (*n));

	}

	assert( s->sp == s->base );

	delete_Stack(s);
	return 0;

}

#endif
