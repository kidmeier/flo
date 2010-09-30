#ifndef __data_list_h__
#define __data_list_h__

#include <stdlib.h> // NULL

#include "core.types.h"
#include "mm.region.h"

// Types

typedef struct List_Node List_Node;
struct List_Node {

	List_Node* prev;
	List_Node* next;

	uchar      value[];

};

typedef struct List List;
struct List {

	region_p   region;
	uint       li_size;

	List_Node* pool;

	List_Node* head;
	List_Node* tail;

};

// Construction
List*              List_from( region_p R, uint li_size, ... );

// Instantiation
List*        alloc_List( region_p R );
List*         init_List( List* list, region_p R, uint li_size );
List*          new_List( region_p R, uint li_size );
void        delete_List( List* list );

pointer        new_List_item( List* list );
pointer        dup_List_item( List* list, const pointer li );
void          free_List_item( List* list, pointer li );

// Functions
pointer       item_List_Node( const List_Node* node );
List_Node*         List_Node_item( const pointer li );

pointer      first_List( const List* list );
pointer       last_List( const List* list );
pointer       next_List( const pointer li );
pointer       prev_List( const pointer li );
pointer        nth_List( const List* list, int n );

pointer       find_List( const List* list, bool (*predicate)(const pointer) );

// Predicates
bool       isempty_List( const List* list );
bool        istail_List( const pointer li );
bool        ishead_List( const pointer li );

// Mutators
pointer  pop_front_List( List* list );
pointer   pop_back_List( List* list );
void    push_front_List( List* list, pointer li );
void     push_back_List( List* list, pointer li );
void insert_before_List( List* list, pointer before, pointer li );
void        remove_List( List* list, pointer li );

// Macros
#define      find__List( list, node, pred )	  \
	do { \
		(node) = item_List_Node( (list)->head ); \
		while( ! istail_List( (node) ) ) { \
			if( pred ) { \
				break; \
			} \
			(node) = next_List( (node) ); \
		} \
	} while( 0 )

#endif
