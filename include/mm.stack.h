#ifndef __mm_stack_h__
#define __mm_stack_h__

#include "core.types.h"
#include "mm.zone.h"

typedef struct Stack Stack;

// Instantiations
Stack*  new_Stack( zone_p Z, int sz );
void    delete_Stack( Stack* stack );

// Functions
pointer tell_Stack( const Stack* stack );
size_t  capacity_Stack( const Stack* stack );
size_t  remaining_Stack( const Stack* stack );

// Mutators
pointer push_Stack( Stack* stack, int sz );
pointer dup_Stack( Stack* stack, int sz, pointer p );
pointer pop_Stack( Stack* stack, int sz );

pointer grow_Stack( Stack* stack, int new_sz );


#endif
