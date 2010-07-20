#include "data.list.h"

#ifdef __data_list_TEST__

#include <stdio.h>
#include <stdlib.h>

struct node_s {
	int n;
	llist_mixin( struct node_s );
};

static void print_list( const char* name, struct node_s* head ) {

	printf("%s: ", name);

	struct node_s* node = head;
	while( !llist_istail(node) ) {

		printf("%d ", node->n);
		node = llist_next(node);

	}
	printf("\n");

}

static void freenode( struct node_s* node ) {

	printf("free( { .n = %d } )\n", node->n);
	free(node);

}


int main( int argc, char* argv[] ) {

	llist( struct node_s, L );

	print_list("L", L);

	// 0 .. 9
	for( int i=9; i>=0; i-- ) {

		struct node_s* n = (struct node_s*)malloc( sizeof(struct node_s) );
		n->n = i;

		printf("llist_push_front(L,%d)\n", n->n);
		llist_push_front( L, n );
		
	}

	print_list("L", L);

	// List at 0, 3, 9, 10
	printf("llist_at(struct node_s, L, 0)->n = %d\n", llist_at(struct node_s, L,0)->n);
	printf("llist_at(struct node_s, L, 3)->n = %d\n", llist_at(struct node_s, L,3)->n);
	printf("llist_at(struct node_s, L, 9)->n = %d\n", llist_at(struct node_s, L,9)->n);

	// Find 8
	struct node_s* eight = NULL;
	llist_find( L, eight, eight->n == 8 );

	printf("llist_find(L, eight, eight->n == 8);\n");
	print_list("eight", eight);

	// Insert 10 before eight
	struct node_s* ten = (struct node_s*)malloc( sizeof(struct node_s) );
	ten->n = 10;
	llist_insert_at( L, eight, ten );

	printf("llist_insert_at(L, eight, ten);\n");
	print_list("L", L);

	// Find tail
	struct node_s* tail = NULL;
	llist_find( L, tail, llist_istail(tail) );

	printf("llist_find(L, tail, llist_istail(tail));\n");
	print_list("tail", tail);

	// Insert 11 at end
	struct node_s* eleven = (struct node_s*)malloc( sizeof(struct node_s) );
	eleven->n = 11;
	llist_insert_at( L, tail, eleven );

	printf("llist_insert_at(L, tail, eleven);\n");
	print_list("L", L);

	llist_destroy(L, freenode);

}

#endif
