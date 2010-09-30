#include <assert.h>

#include "control.maybe.h"
#include "core.log.h"
#include "data.list.h"
#include "job.histogram.h"
#include "job.queue.h"
#include "mm.heap.h"
#include "mm.region.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.spinlock.h"

#include "core.alloc.h"

// Job queue //////////////////////////////////////////////////////////////////

static void*       job_pool = NULL;

llist( static Job, free_job_list );
static spinlock_t  free_job_lock;

llist( static Job, job_queue);
static spinlock_t  job_queue_lock;

static mutex_t     job_queue_mutex;
static condition_t job_queue_signal;

static uint32 alloc_id() {

	return (uint32)microseconds();

}

static uint32 init_job( Job* job, 
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

	job->status = jobNew;

	job->waitqueue = NULL;

	llist_init_node( job );

	return job->id;

}

// Public API /////////////////////////////////////////////////////////////////

int init_Job_queue(void) {

	if( !job_pool ) {
		
		int ret = init_SPINLOCK( &job_queue_lock ); 

		ret = maybe(ret, < 0, init_Job_histogram() );
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
	return 0;

}


void  insert_Job( Job* job ) {
	
	lock_SPINLOCK( &job_queue_lock );

	// If the job is not new or blocked, it is already in the runqueues; no-op
	if( (jobBlocked != job->status && jobNew != job->status) ) {
		assert( 0 );
		unlock_SPINLOCK( &job_queue_lock );
		return;
	}

//	trace( "INSERT 0x%x:%x", (unsigned)job, job->id );
	// If its new, update the counts.
	if( jobNew == job->status )
		upd_Job_histogram( job->deadline, 1 );

	// It belongs to us now
	job->status = jobWaiting;

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

	Job* node = NULL;

	llist_find( job_queue, node, job->deadline < node->deadline );
	llist_insert_at( job_queue, node, job );

	unlock_SPINLOCK( &job_queue_lock );
	assert( jobWaiting == job->status );

}

jobid alloc_Job( uint32 deadline, jobclass_e jobclass, void* result_p, jobfunc_f run, void* params ) {

	Job* job = NULL;

	// Resurrect a job from the completed list
	lock_SPINLOCK( &free_job_lock );

	if( !llist_isempty(free_job_list) ) {

		llist_pop_front( free_job_list, job );
		unlock_SPINLOCK( &free_job_lock );

	} else {

		unlock_SPINLOCK( &free_job_lock );

		// Need a whole new one
		job = new(job_pool, Job);

		job->R = region( "job.queue::alloc_Job" );
		init_SPINLOCK( &job->waitqueue_lock );

	}

	// Configure
	uint32 id = init_job( job, deadline, jobclass, result_p, run, params );

	// Return the jobid
	return (jobid) { id, job };

}

void free_Job( Job* job ) {

	lock_SPINLOCK( &job_queue_lock );
	  upd_Job_histogram( job->deadline, -1 );
	unlock_SPINLOCK( &job_queue_lock );

	rcollect( job->R );

	lock_SPINLOCK( &free_job_lock );
	  llist_push_front( free_job_list, job );
	unlock_SPINLOCK( &free_job_lock );

}

Job* dequeue_Job( usec_t timeout ) {

	Job* job = NULL;

	lock_SPINLOCK( &job_queue_lock );
	  llist_pop_front( job_queue, job );

	  if( NULL == job && 0 != timeout ) {

		  	lock_MUTEX( &job_queue_mutex );
			// Interleave the lock so that no one can change job_queue between the time
			// we check NULL above and here
			unlock_SPINLOCK( &job_queue_lock );

			timed_wait_CONDITION( timeout, &job_queue_signal, &job_queue_mutex );
			unlock_MUTEX( &job_queue_mutex );

			// Now try again, but don't wait (we already have)
			return dequeue_Job( 0 );

	  } else
		  unlock_SPINLOCK( &job_queue_lock );

	return job;
}

// Waitqueues /////////////////////////////////////////////////////////////////

void wakeup_waitqueue_Job( spinlock_t* wq_lock, Waitqueue* waitqueue ) {

	if( wq_lock ) lock_SPINLOCK( wq_lock );

	Job* job; slist_pop_front( *(waitqueue), job );

	while( job ) {
		
		trace( "WAKEUP 0x%x:%x from queue 0x%x", (unsigned)job, job->id, (unsigned)waitqueue );
		insert_Job( job );
		slist_pop_front( *(waitqueue), job);

	}

	if( wq_lock ) unlock_SPINLOCK( wq_lock );
}

void sleep_waitqueue_Job( spinlock_t* wq_lock, Waitqueue* waitqueue, Job* waiting ) {

	if( wq_lock ) lock_SPINLOCK( wq_lock );

	trace( "WAIT 0x%x:%x on 0x%x:%x", (unsigned)waiting, waiting->id, 
	       (unsigned)((char*)waitqueue - ofs_of(Job, waitqueue)),
	       *field_ofs( (char*)waitqueue - ofs_of(Job, waitqueue),
	                   ofs_of(Job, id),
	                   uint32) );

	slist_push_front( *(waitqueue), waiting );
	
	if( wq_lock ) unlock_SPINLOCK( wq_lock );

}
