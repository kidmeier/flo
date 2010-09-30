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

static region_p      job_pool = NULL;

static List*       free_job_list;
static spinlock_t  free_job_lock;

static List*       job_queue;
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

	assert( isempty_List(job->waitqueue) );

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

		job_pool = region( "job.queue" );
		if( !job_pool )
			return -1;

		job_queue     = new_List( job_pool, sizeof(Job) );
		free_job_list = new_List( job_pool, sizeof(Job) );

		return ret;
	}
	return 0;

}


void  insert_Job( Job* job ) {
	
	lock_SPINLOCK( &job_queue_lock );

	// If the job is not new or blocked, it is already in the runqueues; no-op
	if( (jobBlocked != job->status && jobNew != job->status) ) {
		unlock_SPINLOCK( &job_queue_lock );
		return;
	}

	trace( "INSERT 0x%x:%x", (unsigned)job, job->id );

	// If its new, update the counts.
	if( jobNew == job->status )
		upd_Job_histogram( job->deadline, 1 );

	// It belongs to us now
	job->status = jobWaiting;

	// Empty queue
	if( isempty_List(job_queue) ) {
		// Interleave the lock so that if someone enters queue_wait
		// at the same time we get here, we make sure they receive 
		// the broadcast
		lock_MUTEX( &job_queue_mutex );

		push_front_List( job_queue, job );

		unlock_SPINLOCK( &job_queue_lock );

		// Signal anyone waiting for jobs
		broadcast_CONDITION( &job_queue_signal );
		unlock_MUTEX( &job_queue_mutex );

		return;
	}

	Job* node = NULL;

	find__List( job_queue, node, job->deadline < node->deadline );
	insert_before_List( job_queue, node, job );

	unlock_SPINLOCK( &job_queue_lock );
	assert( jobWaiting == job->status );

}

Handle alloc_Job( uint32 deadline, jobclass_e jobclass, void* result_p, jobfunc_f run, void* params ) {

	Job* job = NULL;

	// Resurrect a job from the completed list
	lock_SPINLOCK( &free_job_lock );

	if( !isempty_List(free_job_list) ) {

		job = pop_front_List( free_job_list );
		unlock_SPINLOCK( &free_job_lock );

	} else {

		unlock_SPINLOCK( &free_job_lock );

		// Need a whole new one
		job = new_List_item( free_job_list );

		job->R = region( "job.queue::alloc_Job" );

		init_SPINLOCK( &job->waitqueue_lock );
		job->waitqueue = new_List( job_pool, sizeof(Job) );

	}

	// Configure
	uint32 id = init_job( job, deadline, jobclass, result_p, run, params );

	// Return the handle
	return mk_Handle( id, job );

}

void free_Job( Job* job ) {

	lock_SPINLOCK( &job_queue_lock );
	  upd_Job_histogram( job->deadline, -1 );
	unlock_SPINLOCK( &job_queue_lock );

	rcollect( job->R );

	lock_SPINLOCK( &free_job_lock );
	push_front_List( free_job_list, job );
	unlock_SPINLOCK( &free_job_lock );

}

Job* dequeue_Job( usec_t timeout ) {

	Job* job = NULL;

	lock_SPINLOCK( &job_queue_lock );
	job = pop_front_List( job_queue );

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

	Job* job = pop_front_List( *(waitqueue) );
	
	while( job ) {
		
		trace( "WAKEUP 0x%x:%x from queue 0x%x", (unsigned)job, job->id, (unsigned)waitqueue );
		insert_Job( job );
		job = pop_front_List( *(waitqueue) );

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

	// Push onto the waitqueue and mark as blocked
	waiting->status = jobBlocked;
	push_back_List( *(waitqueue), waiting );
	
	if( wq_lock ) unlock_SPINLOCK( wq_lock );

}
