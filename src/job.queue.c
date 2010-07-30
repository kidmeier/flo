#include "control.maybe.h"
#include "data.list.h"
#include "job.histogram.h"
#include "job.queue.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.spinlock.h"

#include "core.alloc.h"

// Job queue //////////////////////////////////////////////////////////////////

static void*       job_pool = NULL;

llist( static job_queue_t, free_job_list );
static spinlock_t  free_job_lock;

llist( static job_queue_t, job_queue);
static spinlock_t  job_queue_lock;

static mutex_t     job_queue_mutex;
static condition_t job_queue_signal;

static uint32 alloc_id() {

	return (uint32)microseconds();

}

static uint32 init_job( job_queue_p job, 
                        job_queue_p parent,
                        uint32 deadline, 
                        jobclass_e jobclass, 
                        void* result_p,
                        jobfunc_f run, 
                        void* params ) {
	
	job->id = alloc_id();
	init_fibre( &job->fibre );
	
	job->deadline = deadline;
	job->jobclass = jobclass;

	job->result_p = result_p;
	job->run = run;
	job->params = params;
	job->locals = NULL;

	job->status = jobWaiting;

	job->parent = parent;
	job->waitqueue = NULL;

	llist_init_node( job );

	return job->id;

}

// Public API /////////////////////////////////////////////////////////////////

int init_JOB_queue(void) {

	if( !job_pool ) {
		
		int ret = init_SPINLOCK( &job_queue_lock ); 

		ret = maybe(ret, < 0, init_JOB_histogram() );
		ret = maybe(ret, < 0, init_SPINLOCK( &free_job_lock ));
		
		ret = maybe(ret, < 0, init_MUTEX( &job_queue_mutex ));
		ret = maybe(ret, < 0, init_CONDITION( &job_queue_signal ));

		if( ret < 0 )
			return ret;

		job_pool = autofree_pool();
		if( !job_pool )
			return -1;

		return ret;
	}
}


void  insert_JOB( job_queue_p job ) {
	
	lock_SPINLOCK( &job_queue_lock );

	// If the job is being woken up, don't update the histogram
	// as it has already been accounted for.
	if( jobBlocked != job->status )
		upd_JOB_histogram( job->deadline, 1 );

	// Empty queue
	if( llist_isempty(job_queue) ) {
		// Interleave the lock so that if someone enters queue_wait
		// at the same time we get here, we make sure they receive 
		// the broadcast
		lock_MUTEX( &job_queue_mutex );

		llist_push_front( job_queue, job );

		unlock_SPINLOCK( &job_queue_lock );

		// Signal anyone waiting for jobs
		broadcast_CONDITION( &job_queue_signal );
		unlock_MUTEX( &job_queue_mutex );

		return;
	}

	job_queue_p node = NULL;

	llist_find( job_queue, node, job->deadline < node->deadline );
	llist_insert_at( job_queue, node, job );

	unlock_SPINLOCK( &job_queue_lock );

}

jobid alloc_JOB( job_queue_p parent, uint32 deadline, jobclass_e jobclass, void* result_p, jobfunc_f run, void* params ) {

	job_queue_p job = NULL;

	// Resurrect a job from the completed list
	lock_SPINLOCK( &free_job_lock );

	if( !llist_isempty(free_job_list) ) {

		llist_pop_front( free_job_list, job );
		unlock_SPINLOCK( &free_job_lock );

	} else {

		unlock_SPINLOCK( &free_job_lock );

		// Need a whole new one
		job = new(job_pool, job_queue_t);
		init_SPINLOCK( &job->waitqueue_lock );

	}

	// Configure
	uint32 id = init_job( job, parent, deadline, jobclass, result_p, run, params );

	// Return the jobid
	return (jobid) { id, job };

}

void free_JOB( job_queue_p job ) {

	lock_SPINLOCK( &job_queue_lock );
	  upd_JOB_histogram( job->deadline, -1 );
	unlock_SPINLOCK( &job_queue_lock );

	lock_SPINLOCK( &free_job_lock );
	  llist_push_front( free_job_list, job );
	unlock_SPINLOCK( &free_job_lock );

}

job_queue_p dequeue_JOB( usec_t timeout ) {

	job_queue_p job = NULL;

	lock_SPINLOCK( &job_queue_lock );
	  llist_pop_front( job_queue, job );

	  if( NULL == job && 0 != timeout ) {

		  	lock_MUTEX( &job_queue_mutex );
			// Interleave the lock so that no one can change job_queue between the time
			// we check NULL above and here
			unlock_SPINLOCK( &job_queue_lock );

			int rc = timed_wait_CONDITION( timeout, &job_queue_signal, &job_queue_mutex );
			unlock_MUTEX( &job_queue_mutex );

			// Now try again, but don't wait (we already have)
			return dequeue_JOB( 0 );

	  } else
		  unlock_SPINLOCK( &job_queue_lock );

	return job;
}

// Waitqueues /////////////////////////////////////////////////////////////////

void wakeup_waitqueue_JOB( spinlock_t* wq_lock, job_queue_p* waitqueue ) {

	lock_SPINLOCK( wq_lock );

	job_queue_p job; slist_pop_front( *(waitqueue), job );

	while( job ) {
		
		insert_JOB( job );
		slist_pop_front( *(waitqueue), job);

	}

	unlock_SPINLOCK( wq_lock );
}

void sleep_waitqueue_JOB( spinlock_t* wq_lock, job_queue_p* waitqueue, job_queue_p waiting ) {

	if( wq_lock ) lock_SPINLOCK( wq_lock );
      slist_push_front( *(waitqueue), waiting );
  if( wq_lock ) unlock_SPINLOCK( wq_lock );
}

