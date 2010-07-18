#ifndef __data_list_h__
#define __data_list_h__

#include <stdlib.h> // NULL

// Doubly-linked list /////////////////////////////////////////////////////////

#define llist_mixin( type )	\
	type * next; \
	type * prev

#define llist_init( node )	\
	do { \
		node->next = NULL; \
		node->prev = NULL; \
	} while( 0 )

#define llist_isempty( head ) \
	(NULL == head)

#define llist_at( type, head, n )	  \
	(type * )_llist_at( (head), (n), ofs_of( (type), next ) )

#define llist_find( head, node, pred )	  \
	do { \
		node = head; \
		while( NULL != node ) { \
			if( pred ) { \
				break; \
			} \
			node = node->next; \
		} \
	} while( 0 )

#define llist_push_front( head, node ) \
	do { \
		if( head ) \
			head ->prev = node; \
		node->next = head; \
		node->prev = NULL; \
		head = node; \
	} while ( 0 )

#define llist_pop_front( head, front )	  \
	do { \
		front = head; \
		if( NULL != head ) { \
			head = head ->next; \
			if( head ) \
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
	

// Helpers ////////////////////////////////////////////////////////////////////
//// Users of the list mixin should not call these helper funcs. They are for
///  the macro implementations' use.
static inline
const void* _llist_at( void* head, int n, int next_ofs ) {

	const void* node = head;
	for( int i=0; i<n; i++ ) { 
		node = node + next_ofs; 
		if( !node )
			return NULL;
	}

	return node;

}

#endif
