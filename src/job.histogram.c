#include "data.list.h"
#include "job.histogram.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.spinlock.h"

#include "core.alloc.h"

struct histogram_s {

	uint32 deadline;
	int    count;

	mutex_t*     mutex;
	condition_t* signal;

	llist_mixin( struct histogram_s );

};

llist( static struct histogram_s, deadline_histogram );
llist( static struct histogram_s, free_histogram_list );;

static spinlock_t          free_histogram_lock;

static struct histogram_s* alloc_histogram( uint32 deadline ) {

	struct histogram_s* hist = NULL;

	lock_SPINLOCK( &free_histogram_lock );
	if( ! llist_isempty(free_histogram_list) ) {

		llist_pop_front( free_histogram_list, hist );
		unlock_SPINLOCK( &free_histogram_lock );

	} else {

		unlock_SPINLOCK( &free_histogram_lock );
		hist = new( NULL, struct histogram_s );

	}

	hist->deadline = deadline;
	hist->count = 0;

	hist->mutex = NULL;
	hist->signal = NULL;

	llist_init_node( hist );

	return hist;

}

static void insert_histogram( struct histogram_s* hist ) {

	if( llist_isempty(deadline_histogram) ) {

		llist_push_front( deadline_histogram, hist );
		return;

	}

	struct histogram_s* node = NULL;
	llist_find( deadline_histogram, node, hist->deadline < node->deadline );
	llist_insert_at( deadline_histogram, node, hist );

}

static void free_histogram( struct histogram_s* hist ) {

	lock_SPINLOCK( &free_histogram_lock );

	llist_remove( deadline_histogram, hist );
	llist_push_front( free_histogram_list, hist );

	// Notify if anyone is waiting on this
	if( hist->mutex && hist->signal ) {

		unlock_SPINLOCK( &free_histogram_lock );

		lock_MUTEX( hist->mutex );
		broadcast_CONDITION( hist->signal );
		unlock_MUTEX( hist->mutex );

	} else 
		unlock_SPINLOCK( &free_histogram_lock );

}

static struct histogram_s* find_histogram( uint32 deadline ) {

	// Find the histogram node
	struct histogram_s* node = NULL;
	llist_find( deadline_histogram, node, node->deadline == deadline );

	return node;

}

// Public API

int init_JOB_histogram(void) {
	
	return init_SPINLOCK( &free_histogram_lock );

}

int upd_JOB_histogram( uint32 deadline, int incr ) {

	struct histogram_s* node = find_histogram(deadline);

	// If not found allocate one
	if( llist_istail(node) ) {
		node = alloc_histogram(deadline);
		insert_histogram(node);
	}

	// Update the count
	node->count += incr;

	// Free if count drops to zero or below 
	if( node->count <= 0 ) {

		free_histogram(node);

	}

}

int wait_JOB_histogram( uint32 deadline, mutex_t* mutex, condition_t* signal ) {

	lock_SPINLOCK( &free_histogram_lock );

	struct histogram_s* hist = find_histogram(deadline);
	
	if( !hist ) {

		// If histogram is empty then return error
		if( llist_isempty(deadline_histogram) ) {
			unlock_SPINLOCK( &free_histogram_lock );
			return -1;
		}

		// If we are asking to wait on a deadline that precedes the first
		// in our histogram, we assume that all jobs in that deadline have
		// completed and the histogram has been freed
		if( deadline < deadline_histogram->deadline ) {
			unlock_SPINLOCK( &free_histogram_lock );
			return 0;
		}

		unlock_SPINLOCK( &free_histogram_lock );

		// Otherwise signal an error
		return -1;

	}

	// If someone is already waiting on this histogram signal an error
	// (don't support this yet, not sure if its needed)
	if( hist->mutex || hist->signal ) {

		unlock_SPINLOCK( &free_histogram_lock );
		return -1;

	}

	// Otherwise do the wait
	hist->mutex = mutex;
	hist->signal = signal;

	unlock_SPINLOCK( &free_histogram_lock );

	wait_CONDITION( signal, mutex );

	return 0;

}
