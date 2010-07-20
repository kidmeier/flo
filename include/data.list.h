#ifndef __data_list_h__
#define __data_list_h__

#include <stdlib.h> // NULL

#include "core.types.h"

// Types
typedef void (*llist_freenode_f)( void* );

// Doubly-linked list /////////////////////////////////////////////////////////

#define llist_mixin( type )	\
	type * next; \
	type * prev

#define llist(type, name)	  \
	type name##_tail = { .next = NULL, .prev = NULL }; \
	type * name = & name##_tail

#define llist_init_node( node )	\
	do { \
		node->next = NULL; \
		node->prev = NULL; \
	} while( 0 )

#define llist_next(node) \
	(node)->next

#define llist_prev(node) \
	(node)->prev

#define llist_isempty( head ) \
	( NULL == (head)->next && NULL == (head)->prev)

#define llist_istail( node ) \
	( NULL == (node)->next )

#define llist_at( type, head, n )	  \
	((type * )_llist_at( (head), (n), ofs_of( type, next ) ))

#define llist_find( head, node, pred )	  \
	do { \
		node = head; \
		while( ! llist_istail( node ) ) { \
			if( pred ) { \
				break; \
			} \
			node = node->next; \
		} \
	} while( 0 )

#define llist_push_front( head, node ) \
	do { \
		(head)->prev = node; \
		(node)->next = head; \
		(node)->prev = NULL; \
		(head) = (node); \
	} while ( 0 )

#define llist_pop_front( head, front )	  \
	do { \
		if( ! llist_isempty( (head) )) { \
			front = head; \
			head = head ->next; \
			head ->prev = NULL; \
			front ->next = NULL; \
			front ->prev = NULL; \
		} \
	} while( 0 )

#define llist_insert_at( head, where, node )	  \
	do { \
		node->prev = (where ? where->prev : NULL); \
		node->next = where; \
		if( node->prev ) \
			node->prev->next = node; \
		if( node->next ) \
			node->next->prev = node; \
	  \
		if( head == where ) \
			head = node; \
	} while(0)

#define llist_remove( head, node )	\
	do { \
		if( llist_isempty(head) ) \
			break; \
		if( head == node ) \
			head = node ->next; \
	  \
		if( node ->prev ) { \
			node ->prev->next = node ->next; \
			node ->prev = NULL; \
		} \
	  \
		if( node ->next ) { \
			node ->next->prev = node ->prev; \
			node ->next = NULL; \
		} \
	} while( 0 )
	
#define llist_destroy( head, freenode )	  \
	do { \
		int next_ofs = (void*)&head->next - (void*)head; \
		void* node = (void*)head; \
		void* next = *(void**)(node + next_ofs); \
		while( next ) { \
			(freenode)( node ); \
			node = next; \
			next = *(void**)(node + next_ofs); \
		} \
	} while( 0 )
	
// Helpers ////////////////////////////////////////////////////////////////////
//// Users of the list mixin should not call these helper funcs. They are for
///  the macro implementations' use.
static inline
const void* _llist_at( void* head, int n, int next_ofs ) {

	const void* node = head;
	const void* next = *(void**)(node + next_ofs);

	for( int i=0; i<n; i++ ) { 
		node = next;
		next = *(void**)(node + next_ofs);
		if( NULL == next ) 
			return NULL;
	}

	return node;

}

#endif
