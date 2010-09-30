#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#include "data.list.h"
#include "mm.region.h"

List*   List_from( region_p R, uint li_size, ... ) {

	List* list = new_List( R, li_size );

	va_list items; va_start( items, li_size );
	pointer item = va_arg( items, pointer );
	while( item ) {
		pointer li = dup_List_item( list, item );
		push_back_List( list, li );
		
		item = va_arg( items, pointer );
	}
	va_end(items);

	return list;

}

List*        alloc_List( region_p r ) {

	region_p R = (NULL != r) ? r : region( "list" );
	List* list = ralloc( R, sizeof(List) );
	
	return list;

}

List*         init_List( List* list, region_p R, uint li_size ) {

	list->region  = R;
	list->li_size = li_size;
	
	list->pool = NULL;
	list->tail = list->head = List_Node_item( new_List_item( list ) );

	list->head->prev = NULL;
	list->tail->next = NULL;

	return list;

}

List*          new_List( region_p R, uint li_size ) {

	return init_List( alloc_List(R), R, li_size );

}

void        delete_List( List* list ) {

	rcollect( list->region );
	rfree( list->region );

}

pointer        new_List_item( List* list ) {

	if( list->pool ) {

		List_Node* node = list->pool;
		list->pool = list->pool->next;
		node->next = NULL;

		return item_List_Node(node);

	} else {
	
		return item_List_Node( ralloc( list->region, sizeof(List_Node) + list->li_size ) );
		
	}

}

pointer        dup_List_item( List* list, const pointer li ) {

	pointer item = new_List_item( list );
	memcpy( item, li, list->li_size );

	return item;

}

void          free_List_item( List* list, pointer li ) {

	List_Node* node = List_Node_item(li);
	
	node->prev = NULL;
	node->next = list->pool;
	list->pool = node;

}

// Functions
pointer       item_List_Node( const List_Node* node ) {

	return node ? (pointer)node + offsetof( List_Node, value ) : NULL;

}

List_Node*         List_Node_item( const pointer li ) {

	return (List_Node*)(li ? li - offsetof( List_Node, value ) : NULL);

}

pointer      first_List( const List* list ) {

	return item_List_Node( list->head );

}

pointer       last_List( const List* list ) {

	return item_List_Node( list->tail->prev );

}

pointer       next_List( const pointer li ) {

	return item_List_Node( List_Node_item(li)->next );

}

pointer       prev_List( const pointer li ) {

	return item_List_Node( List_Node_item(li)->prev );

}

pointer        nth_List( const List* list, int n ) {
	
	List_Node* node = list->head;
	for( int i=0; i<n; i++ ) {

		if( node == list->tail )
			return NULL;
		node = node->next;

	}

	return item_List_Node(node);

}

pointer       find_List( const List* list, bool (*predicate)(const pointer) ) {

	pointer node = first_List(list);
	while( node ) {

		if( (*predicate)(node) )
			return node;
		node = next_List(node);

	}
	return NULL;

}

// Predicates
bool       isempty_List( const List* list ) {
	
	return list->head == list->tail;

}

bool        istail_List( const pointer li ) {

	return li ? NULL == List_Node_item(li)->next : false;

}

bool        ishead_List( const pointer li ) {

	return li ? NULL == List_Node_item(li)->prev : false;

}

// Mutators
pointer  pop_front_List( List* list ) {

	if( !isempty_List(list) ) { 
		List_Node* front = list->head;

		list->head = list->head->next;
		list->head->prev = NULL;
		
		front->next = NULL;
		front->prev = NULL;

		return item_List_Node(front);
	}

	return NULL;

}


pointer   pop_back_List( List* list ) {

	if( !isempty_List(list) ) {

		pointer* back = item_List_Node(list->tail->prev);

		remove_List( list, back );
		return back;

	}

	return NULL;

}

void    push_front_List( List* list, pointer li ) {

	List_Node* node = List_Node_item(li);

	node->next = list->head;
	node->prev = NULL;

	list->head->prev = node;
	list->head = node;

}

void     push_back_List( List* list, pointer li ) {

	List_Node* node = List_Node_item(li);

	// If we are empty then just push front; less NULL checks are required.
	if( isempty_List(list) ) {
		push_front_List( list, li );
		return;
	}

	node->prev = list->tail->prev;
	node->next = list->tail;

	list->tail->prev->next = node;
	list->tail->prev = node;

}

void insert_before_List( List* list, pointer before_li, pointer li ) {

	List_Node* node = List_Node_item(li);
	List_Node* before = List_Node_item(before_li);

	node->prev = ( (before) ? (before)->prev : NULL);
	node->next = (before);
	if( node->prev )
		node->prev->next = node;
	if( node->next )
		node->next->prev = node;
	
	if( list->head == (before) )
		list->head = node;

}

void        remove_List( List* list, pointer li ) {

	List_Node* node = List_Node_item(li);

	if( isempty_List(list) || istail_List(li) )
		return;
	
	if( list->head == node )
		list->head = node->next;
	
	if( node->prev )
		node->prev->next = node->next;
	
	if( node->next )
		node->next->prev = node->prev;
	
	node->prev = NULL;
	node->next = NULL;

}

#ifdef __data_list_TEST__

#include <stdio.h>
#include <stdlib.h>

#include "mm.heap.h"

static void print_list( const char* name, pointer li ) {

	printf("%s: ", name);
	while( !istail_List(li) ) {

		printf("%d ", *(int*)li);
		li = next_List(li);

	}
	printf("\n");

}

int main( int argc, char* argv[] ) {

	List* L1 = List_from( sizeof(char*),
	                      &"one",
	                      &"two",
	                      &"three",
	                      &"four",
	                      NULL );
	print_list( "L1", first_List(L1) );

	List* L = new_List( ZONE_heap, sizeof(int) );

	print_list("L", first_List(L));

	// 0 .. 9
	for( int i=9; i>=0; i-- ) {
		
		pointer li = new_List_item( L );
		*(int*)li = i;

		printf("llist_push_front(L,%d)\n", *(int*)li);
		push_front_List( L, li );
		
	}

	print_list("L", first_List(L));

	// List at 0, 3, 9, 10
	printf("llist_at(struct node_s, L, 0)->n = %d\n", *(int*)nth_List(L,0));
	printf("llist_at(struct node_s, L, 3)->n = %d\n", *(int*)nth_List(L,3));
	printf("llist_at(struct node_s, L, 9)->n = %d\n", *(int*)nth_List(L,9));

	// Find 8
	pointer eight = NULL;
	find__List( L, eight, *(int*)eight == 8 );

	printf("llist_find(L, eight, *(int*)eight == 8);\n");
	print_list("eight", eight);

	// Insert 10 before eight
	pointer ten = new_List_item( L );
	*(int*)ten = 10;
	insert_before_List( L, eight, ten );

	printf("llist_insert_at(L, eight, ten);\n");
	print_list("L", first_List(L));

	// Find tail
	pointer* tail = NULL;
	find__List( L, tail, istail_List(tail) );

	printf("llist_find(L, tail, llist_istail(tail));\n");
	print_list("tail", tail);

	// Insert 11 at end
	pointer eleven = new_List_item(L);
	*(int*)eleven = 11;
	insert_before_List( L, tail, eleven );

	printf("llist_insert_at(L, tail, eleven);\n");
	print_list("L", first_List(L));

	// Remove head
	pointer node = first_List(L);
	remove_List( L, node );

	printf("llist_remove(L, head);\n");
	print_list("L", first_List(L));

	// Remove 5
	pointer five = NULL;
	find__List( L, five, *(int*)five == 5 );
	remove_List( L, five );

	printf("llist_find( L, five, five->n == 5 );\n");
	printf("llist_remove( L, five );\n");
	print_list("L", first_List(L));

	// Remove tail
	find__List( L, tail, 0 );
	remove_List( L, tail );

	printf("llist_find(L, tail, 0);\n");
	printf("llist_remove( L, tail );\n");
	print_list("L", first_List(L));

	delete_List(L);

}

#endif
