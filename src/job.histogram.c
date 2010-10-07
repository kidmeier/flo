#include <assert.h>

#include "data.list.h"
#include "job.histogram.h"
#include "mm.region.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.spinlock.h"

struct histogram_s {

	uint32 deadline;
	int    count;

	mutex_t*     mutex;
	condition_t* signal;

};

region_p     pool = NULL;

static List* deadline_histogram;
static List* free_histogram_list;

static spinlock_t          histogram_lock;
static spinlock_t          free_histogram_lock;

static struct histogram_s* alloc_histogram( uint32 deadline ) {

	struct histogram_s* hist = NULL;

	lock_SPINLOCK( &free_histogram_lock );
	if( !isempty_List(free_histogram_list) ) {

		hist = pop_front_List( free_histogram_list );

		unlock_SPINLOCK( &free_histogram_lock );

	} else {

		hist = new_List_item( free_histogram_list );
		unlock_SPINLOCK( &free_histogram_lock );

	}

	hist->deadline = deadline;
	hist->count = 0;

	hist->mutex = NULL;
	hist->signal = NULL;

	return hist;

}

static void insert_histogram( struct histogram_s* hist ) {

	if( isempty_List(deadline_histogram) ) {

		push_front_List( deadline_histogram, hist );
		return;

	}

	struct histogram_s* node = NULL;
	find__List( deadline_histogram, node, hist->deadline < node->deadline );
	insert_before_List( deadline_histogram, node, hist );

}

static void free_histogram( struct histogram_s* hist ) {

	remove_List( deadline_histogram, hist );

	lock_SPINLOCK( &free_histogram_lock );

	push_front_List( free_histogram_list, hist );

	// Notify if anyone is waiting on this
	if( hist->mutex && hist->signal ) {

		unlock_SPINLOCK( &free_histogram_lock );

		lock_MUTEX( hist->mutex );
		broadcast_CONDITION( hist->signal );
		unlock_MUTEX( hist->mutex );

	} else {

		unlock_SPINLOCK( &free_histogram_lock );

	}

}

static struct histogram_s* find_histogram( uint32 deadline ) {

	// Find the histogram node
	struct histogram_s* node = NULL;
	find__List( deadline_histogram, node, node->deadline == deadline );

	return node;

}

// Public API

int init_Job_histogram(void) {

	pool = region( "job.histogram::pool" );

	deadline_histogram  = new_List( pool, sizeof(struct histogram_s) );
	free_histogram_list = new_List( pool, sizeof(struct histogram_s) );

	init_SPINLOCK( &histogram_lock );
	init_SPINLOCK( &free_histogram_lock );

	return 0;

}

int upd_Job_histogram( uint32 deadline, int incr ) {

	lock_SPINLOCK( &histogram_lock );
	struct histogram_s* node = find_histogram(deadline);

	// If not found allocate one
	if( istail_List(node) ) {
		node = alloc_histogram(deadline);
		insert_histogram(node);
	}

	// Update the count
	node->count += incr;
	assert( node->count >= 0 );

	// Free if count drops to zero or below 
	if( node->count <= 0 ) {

		free_histogram(node);

	}

	unlock_SPINLOCK( &histogram_lock );
	
	return 0;

}

int wait_Job_histogram( uint32 deadline, mutex_t* mutex, condition_t* signal ) {

	lock_SPINLOCK( &free_histogram_lock );
	lock_SPINLOCK( &histogram_lock );

	struct histogram_s* hist = find_histogram(deadline);
	
	if( !hist ) {

		// If histogram is empty then return error
		if( isempty_List(deadline_histogram) ) {
			unlock_SPINLOCK( &histogram_lock );
			unlock_SPINLOCK( &free_histogram_lock );
			return -1;
		}

		// If we are asking to wait on a deadline that precedes the first
		// in our histogram, we assume that all jobs in that deadline have
		// completed and the histogram has been freed
		struct histogram_s* first = first_List(deadline_histogram);
		if( deadline < first->deadline ) {
			unlock_SPINLOCK( &histogram_lock );
			unlock_SPINLOCK( &free_histogram_lock );
			return 0;
		}

		unlock_SPINLOCK( &histogram_lock );
		unlock_SPINLOCK( &free_histogram_lock );

		// Otherwise signal an error
		return -1;

	}

	// If someone is already waiting on this histogram signal an error
	// (don't support this yet, not sure if its needed)
	if( hist->mutex || hist->signal ) {

		unlock_SPINLOCK( &histogram_lock );
		unlock_SPINLOCK( &free_histogram_lock );
		return -1;

	}

	// Otherwise do the wait
	hist->mutex = mutex;
	hist->signal = signal;

	unlock_SPINLOCK( &histogram_lock );
	unlock_SPINLOCK( &free_histogram_lock );

	wait_CONDITION( signal, mutex );

	return 0;

}
