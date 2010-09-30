#ifndef __data_handle_h__
#define __data_handle_h__

#include "core.types.h"

// Abtracts the idea of a reference to a resource whose lifecycle is controlled
// by means outside the direct control of the handle's holder. 
//
// The handle contains a unique identitifer as well as a pointer to some opaque
// structure whose first member is also a unique identitifer.
//
// A handle is valid as long as the handle's identifier matches that of its
// referant. For the implementor of the resource, the identifier should be
// initialized to a new unique value at the beginning of the resource's
// lifecycle. The macro `nextHid` provides a means to generate a new identifier
// that is available for client's to use, although the identifier could come
// from anywhere.
// 

typedef struct Handle Handle; 

struct Handle {

	uint32  id;
	pointer data;

};

#define mk_Handle(_id, _data)	  \
	((Handle){ .id = (_id), .data = (_data) })

#define isvalid_Handle( hnd ) \
	(hnd).id == *(uint32*)((hnd).data)


#define deref_Handle( typ, hnd ) \
	(( typ * )(hnd).data)

#endif
