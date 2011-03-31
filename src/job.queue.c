#include <assert.h>

#include "core.features.h"
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

static region_p    job_pool = NULL;

static List*       free_job_list;
static spinlock_t  free_job_lock;

static List*       job_queue;
static spinlock_t  job_queue_lock;

static mutex_t     job_queue_mutex;
static condition_t job_queue_signal;

static threadlocal List*       sticky_queue;
static threadlocal spinlock_t  sticky_queue_lock;

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

	job->waitqueue = new_List( job->R, sizeof(Handle) );
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

		job_pool = region( "job.queue.jobs" );
		if( !job_pool )
			return -1;

		job_queue     = new_List( job_pool, sizeof(Job) );
		free_job_list = new_List( job_pool, sizeof(Job) );

		return ret;
	}
	return 0;

}

int init_Job_queue_thread( pointer thread ) {

	sticky_queue = new_List( job_pool, sizeof(Job) );
	return init_SPINLOCK( &sticky_queue_lock );

}

void  insert_Job( Job* job ) {
	
	// Can't insert jobs that are already owned by a runqueue
	assert( jobBlocked == job->status || jobNew == job->status );
	trace( "INSERT 0x%x:%x", (unsigned)job, job->id );

	List*      queue = job_queue;
	spinlock_t* lock = &job_queue_lock;

	// If we are running in a worker thread and the job is sticky
	// insert it on the threadlocal queue
	if( NULL != sticky_queue && stickyJob == job->jobclass ) {

		queue = sticky_queue;
		lock  = &sticky_queue_lock;

	}

	lock_SPINLOCK( lock );

	// Once we're committed to inserting, the job is now waiting, which
	// prevents further inserts
	job->status = jobWaiting;

	// Empty queue
	if( isempty_List(queue) ) {
		// Interleave the lock so that if someone enters queue_wait
		// at the same time we get here, we make sure they receive 
		// the broadcast
		lock_MUTEX( &job_queue_mutex );

		push_front_List( queue, job );

		unlock_SPINLOCK( lock );

		// Signal anyone waiting for jobs
		broadcast_CONDITION( &job_queue_signal );
		unlock_MUTEX( &job_queue_mutex );

		return;
	}

	Job* node = NULL;

	find__List( queue, node, job->deadline < node->deadline );
	insert_before_List( queue, node, job );

	unlock_SPINLOCK( lock );

}

Handle alloc_Job( uint32 deadline, jobclass_e jobclass, void* result_p, jobfunc_f run, void* params ) {

	Job* job = NULL;

	// Resurrect a job from the completed list
	lock_SPINLOCK( &free_job_lock );

	if( !isempty_List(free_job_list) ) {

		job = pop_front_List( free_job_list );
		unlock_SPINLOCK( &free_job_lock );

	} else {

		// Need a whole new one
		job = new_List_item( free_job_list );

		unlock_SPINLOCK( &free_job_lock );

		job->R = region( "job.queue::alloc_Job" );

		init_SPINLOCK( &job->waitqueue_lock );
		init_SPINLOCK( &job->lock );

	}

	lock_SPINLOCK( &job->lock );

	// Configure
	init_job( job, deadline, jobclass, result_p, run, params );

	// Return the handle
	return mk_Handle( job );

}

void free_Job( Job* job ) {

	job->id = 0;
	assert( jobDone == job->status );
	assert( isempty_List(job->waitqueue) );
	rcollect( job->R );

	lock_SPINLOCK( &free_job_lock );
	push_front_List( free_job_list, job );
	unlock_SPINLOCK( &free_job_lock );

	unlock_SPINLOCK( &job->lock );

}

Job* dequeue_Job( usec_t timeout ) {

	Job* job = NULL;

	// Check any jobs on our sticky (threadlocal) queue
	lock_SPINLOCK( &sticky_queue_lock );
	Job* stickyJob = pop_front_List( sticky_queue );
	unlock_SPINLOCK( &sticky_queue_lock );

	if( NULL != stickyJob )
		return stickyJob;

	// Check the global queue
	lock_SPINLOCK( &job_queue_lock );
	job = pop_front_List( job_queue );

	if( NULL == job && 0 != timeout ) {
		
		lock_MUTEX( &job_queue_mutex );
		// Interleave the lock so that no one can change job_queue between 
		// the time we check NULL above and here
		unlock_SPINLOCK( &job_queue_lock );
		
		timed_wait_CONDITION( timeout, &job_queue_signal, &job_queue_mutex );
		unlock_MUTEX( &job_queue_mutex );
		
		// Now try again, but don't wait (we already have)
		return dequeue_Job( 0 );
		
	} else {
		if( job) // The job is now part of the runqueue, lock it up
			lock_SPINLOCK( &job->lock );

		unlock_SPINLOCK( &job_queue_lock );
	}

	return job;

}

// Waitqueues /////////////////////////////////////////////////////////////////

void wakeup_waitqueue_Job( spinlock_t* wq_lock, Waitqueue* waitqueue ) {
	
	if( wq_lock ) lock_SPINLOCK( wq_lock );

	Handle* hdl = pop_front_List( *(waitqueue) );

	while( hdl ) {

		Job* job = deref_Handle(Job,*hdl);
		
		lock_SPINLOCK( &job->lock );
		
		// Job has been run to completion elsewhere?
		if( isvalid_Handle(*hdl) ) {
			
			// Only if it's still blocked
			if( jobBlocked == job->status ) {
				trace( "WAKEUP 0x%x:%x from queue 0x%x", 
				       (unsigned)job, hdl->id, (unsigned)waitqueue );
				insert_Job( job );

				unlock_SPINLOCK( &job->lock );
				break;
			}
			
		}
		
		unlock_SPINLOCK( &job->lock );
				
		hdl = pop_front_List( *(waitqueue) );

	}

	if( wq_lock ) unlock_SPINLOCK( wq_lock );

}

void sleep_waitqueue_Job( spinlock_t* wq_lock, Waitqueue* waitqueue, Job* waiting ) {

	if( wq_lock ) lock_SPINLOCK( wq_lock );

	trace( "WAIT 0x%x:%x on 0x%x:%x", (unsigned)waiting, waiting->id, 
	       (unsigned)((char*)waitqueue - offsetof(Job, waitqueue)),
	       // Find Job* that the waitqueue belongs to
	       //          get the ofs of Job->id
	       //          deref of Job->id
	       *field_ofs( (char*)waitqueue - offsetof(Job, waitqueue),
	                   ofs_of(Job, id),
	                   uint32) );

	// Make a handle to the waiting job
	Handle* handle = new_List_item( *(waitqueue) );

	*handle = mk_Handle(waiting);

	// Mark as blocked
	waiting->status = jobBlocked;

	// Push onto the waitqueue
	push_back_List( *(waitqueue), handle );

	if( wq_lock ) unlock_SPINLOCK( wq_lock );

}
