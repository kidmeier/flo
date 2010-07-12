#ifndef __core_alloc_h__
#define __core_alloc_h__

#include <stdlib.h>
#include <string.h>

#include <ccan/talloc/talloc.h>

#define alloc( ctx, size )			\
  talloc_size( ctx, size )

#define new( ctx,typ )				\
  talloc( ctx, typ )

#define new_array( ctx,typ,n )	\
  talloc_array( ctx, typ, n )

#define clone( typ,src )					\
  (typ *)memcpy( talloc(NULL, typ), src, sizeof( typ ) )

#define clone_into( ctx, typ, src )			\
  (typ *)talloc_memdup( ctx, src, sizeof( typ ) )

#define clone_string( ctx, s )			\
  talloc_strdup( ctx, s )

#define clone_substring( ctx, s, start, n )	\
  talloc_strndup( ctx, &(s)[start], n )

#define delete( p )				\
  talloc_free( p )

#define adopt( parent,child )			\
  talloc_steal( parent, child )

#define abandon(parent,child)			\
  talloc_unlink( parent, child )

// Scoped allocation contexts
//  application, scene, camera/entity, frame, event

#define autofree_pool talloc_autofree_context

#endif
