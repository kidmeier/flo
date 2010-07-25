#include "control.maybe.h"
#include "control.swap.h"
#include "data.list.h"
#include "job.core.h"
#include "job.fibre.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.spinlock.h"
#include "sync.thread.h"
#include "time.core.h"

#include "core.alloc.h"
#include "core.system.h"

struct job_worker_s {

	short      id;
	thread_t   thread;

	struct job_queue_s** wakequeue;
	spinlock_t           lock;

};

struct job_queue_s {

	uint32           id;
	fibre_t          fibre;

	uint32           deadline;
	jobclass_e  jobclass;

	void*            result_p;
	jobfunc_f        run;
	void*            params;
	void*            locals;

	jobstatus_e status;

	struct job_worker_s* worker;
	struct job_queue_s*  blocked;
	struct job_queue_s*  parent;

	llist_mixin( struct job_queue_s );

};

struct histogram_s {

	uint32 deadline;
	int    count;

	mutex_t*     mutex;
	condition_t* signal;

	llist_mixin( struct histogram_s );

};

// Histogram //////////////////////////////////////////////////////////////////
// NOTE: These functions are not implicitly thread safe. It is up to the caller
//       to ensure calls to these functions are performed serially across
//       threads. In other words, callers should lock on a global object
//       before calling these.

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

static int upd_histogram( uint32 deadline, int incr ) {

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

static int wait_histogram( uint32 deadline, mutex_t* mutex, condition_t* signal ) {

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

// Job queue //////////////////////////////////////////////////////////////////

static void*       job_pool = NULL;

llist( static job_queue_t, free_job_list );
static spinlock_t  free_job_lock;

llist( static job_queue_t, job_queue);
static spinlock_t  job_queue_lock;

static mutex_t     job_queue_mutex;
static condition_t job_queue_signal;

static int init_job_queue(void) {

	job_pool = autofree_pool();
	if( !job_pool )
		return -1;
	
	int ret = init_SPINLOCK( &free_histogram_lock );

	ret = maybe(ret, < 0, init_SPINLOCK( &job_queue_lock ));
	ret = maybe(ret, < 0, init_SPINLOCK( &free_job_lock ));

	ret = maybe(ret, < 0, init_MUTEX( &job_queue_mutex ));
	ret = maybe(ret, < 0, init_CONDITION( &job_queue_signal ));

	return ret;
}

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
	job->blocked = NULL;//parent;

	llist_init_node( job );

	return job->id;

}

static void  insert_job( job_queue_p job ) {
	
	lock_SPINLOCK( &job_queue_lock );

	// Update the histogram
	upd_histogram( job->deadline, 1 );

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

static jobid alloc_job( job_queue_p parent, uint32 deadline, jobclass_e jobclass, void* result_p, jobfunc_f run, void* params ) {

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

	}

	// Configure
	uint32 id = init_job( job, parent, deadline, jobclass, result_p, run, params );

	// Insert into the active list
	insert_job( job );

	// Return the jobid
	return (jobid) { id, job };
}

static void free_job( job_queue_p job ) {

	lock_SPINLOCK( &job_queue_lock );
	  upd_histogram( job->deadline, -1 );
	  llist_remove( job_queue, job );
	unlock_SPINLOCK( &job_queue_lock );

	job->blocked = NULL;

	lock_SPINLOCK( &free_job_lock );
	  llist_push_front( free_job_list, job );
	unlock_SPINLOCK( &free_job_lock );

}

static job_queue_p dequeue_job( struct job_worker_s* worker ) {

	job_queue_p job = NULL;

	lock_SPINLOCK( &job_queue_lock );
	  llist_pop_front( job_queue, job );
	unlock_SPINLOCK( &job_queue_lock );

	if( job ) {
		job->worker = worker;
	}

	return job;
}

static void wakeup_job( struct job_worker_s* from, job_queue_p job ) {

	struct job_worker_s* worker = job->worker;

	// We only need to lock if the job to be awoken is owned by a different
	// worker from the one that called us
	if( worker != from ) {

		lock_SPINLOCK( &worker->lock );
		llist_push_front( (*worker->wakequeue), job );
		unlock_SPINLOCK( &worker->lock );

	} else {

		llist_push_front( (*worker->wakequeue), job );

	}
	job->status = jobWaiting;

}

static int queue_wait( uint64 usec ) {

	lock_SPINLOCK( &job_queue_lock );

	if( !llist_isempty(job_queue) ) {
		unlock_SPINLOCK( &job_queue_lock );
		return 0;
	}

	lock_MUTEX( &job_queue_mutex );
	// Interleave the lock so that no one can change job_queue between the time
	// we check NULL above and here
	  unlock_SPINLOCK( &job_queue_lock );
	  int rc = timed_wait_CONDITION( usec, &job_queue_signal, &job_queue_mutex );
	unlock_MUTEX( &job_queue_mutex );
	
	return rc;
}

// Pthread workers ////////////////////////////////////////////////////////////

// TODO:
//  Implement the wakeup_job mechanism. Requires:
//
//  1. job_queue_s needs a way to refer to which thread's sleepqueue it lives on
//  2. a synchronized access to the thread's wakequeue 
//  3. 

static bool job_queue_running = false;
static int schedule_work( struct job_worker_s* self ) {

	llist(job_queue_t, running);
	llist(job_queue_t, expired);
	llist(job_queue_t, sleeping);
	llist(job_queue_t, wakequeue);

	self->wakequeue = &wakequeue;
	unlock_SPINLOCK( &self->lock );

	while( job_queue_running ) {

		// Check for new work
		job_queue_p job = dequeue_job(self);
		if( job ) {

			// Insert it into the runqueue at the appropriate place
			job_queue_p insert_pt = NULL;
			llist_find( running, insert_pt, job->deadline < insert_pt->deadline );
			llist_insert_at( running, insert_pt, job );
			
		} else if( llist_isempty(running) ) {

			// No work to steal and we don't have any in our bucket, wait
			int rc = queue_wait( usec_perSecond );
			continue;

		}

		// Insert woken up jobs into queue
		if( ! llist_isempty( wakequeue ) ) {
			lock_SPINLOCK( &self->lock );
			
			job_queue_p j = NULL; llist_pop_front( wakequeue, j );
			while( j ) {
				job_queue_p insert_pt;

				llist_find( running, insert_pt, j->deadline < insert_pt->deadline );
				llist_insert_at( running, insert_pt, j );

				llist_pop_front( wakequeue, j );
			}

			unlock_SPINLOCK( &self->lock );
		}

		// Run a timeslice
		while( !llist_isempty(running) ) {

			// Take first job from runqueue
			llist_pop_front( running, job );

			job->status = jobRunning;
			char status = job->run(job, job->result_p, job->params, &job->locals);

			if( jobBlocked == status ) { // put it to sleep until notified

//				fprintf(stderr, "[% 2d] Putting to sleep job 0x%x: id=0x%x, deadline=%d\n", 
//				        self->id, (unsigned)job, job->id, job->deadline);
				llist_push_front( sleeping, job );
				job->status = jobBlocked;

			} else if( jobWaiting == status ) { // the thread is polling a condition; expire it

				job_queue_p insert_pt;
//				fprintf(stderr, "[% 2d] Job has expired 0x%x: id=0x%x, deadline=%d\n", 
//				        self->id, (unsigned)job, job->id, job->deadline);
				
				
				llist_find( expired, insert_pt, job->deadline < insert_pt->deadline );
				llist_insert_at( expired, insert_pt, job );
				job->status = jobWaiting;
				
			} else if( jobExited == status     // the thread called exit_job(); notify and free
			           || jobDone == status) { // the thread function completed; notify and free
//				fprintf(stderr, "[% 2d] Job is complete: 0x%x: id=0x%x, deadline=%d\n", 
//				        self->id, (unsigned)job, job->id, job->deadline);
				
				job->status = jobDone;
				if( job->blocked ) {

					wakeup_job( self, job->blocked );

				}
				free_job( job );
			}

		}
		// Move on to next timeslice
		swap( job_queue_p, running, expired );
	}

}

// Public API /////////////////////////////////////////////////////////////////

static int                  n_workers;
static struct job_worker_s* workers;

int   init_JOBS(void) {

	if( init_job_queue() < 0 )
		return -1;
	job_queue_running = true;

	n_workers = cpu_count_SYS();
	workers = new_array( NULL, struct job_worker_s, n_workers );

	for( int i=0; i<n_workers; i++ ) {

		workers[i].id = i;

		int ret = init_SPINLOCK( &workers[i].lock );
		ret = maybe( ret, < 0, lock_SPINLOCK(&workers[i].lock) );
		ret = maybe( ret, < 0, create_THREAD( &workers[i].thread, 
		                                     (threadfunc_f)schedule_work, 
		                                     &workers[i]) );
		ret = maybe( ret, < 0, lock_SPINLOCK(&workers[i].lock) );
		ret = maybe( ret, < 0, unlock_SPINLOCK(&workers[i].lock) );

		if( ret < 0 ) {
			job_queue_running = false;
			for( int j=0; j<i; j++ ) 
				join_THREAD( &workers[j].thread, NULL );
			return ret;
		}

	}

	return 0;

}

void             shutdown_JOBS(void) {

	job_queue_running = false;

	for( int i=0; i<n_workers; i++ ) {
		join_THREAD( &workers[i].thread, NULL );
	}

}


jobid queue_JOB( job_queue_p parent, uint32 deadline, jobclass_e jobclass, void* result_p, jobfunc_f run, void* params ) {

	jobid job = alloc_job( parent, deadline, jobclass, result_p, run, params );
	return job;

}

jobstatus_e status_JOB( jobid jid ) {

	if( jid.id != jid.job->id )
		return jobDone;

	return jid.job->status;

}

int   join_deadline_JOB( uint32 deadline, mutex_t* mutex, condition_t* signal ) {

	return wait_histogram(deadline, mutex, signal);

}

#ifdef __job_core_TEST__

declare_job( fibonacci, unsigned long long,
             unsigned long long n;
	);

define_job( fibonacci, unsigned long long,

            unsigned long long n_1;
            unsigned long long n_2;

            jobid job_n_1;
            jobid job_n_2;

) {
	begin_job;

	if( 0 == arg( n ) ) {

		exit_job( 0 );

	} else if( 1 == arg( n ) ) {

		exit_job( 1 );

	}

	spawn_job( local(job_n_1), arg(n) - 1, cpuBound, &local(n_1), fibonacci, { arg(n) - 1 } );
	spawn_job( local(job_n_2), arg(n) - 2, cpuBound, &local(n_2), fibonacci, { arg(n) - 2 } );
		
	exit_job( local(n_1) + local(n_2) );

	end_job;
}

int main( int argc, char* argv[] ) {

	if( argc < 2 ) {
		fprintf(stderr, "Specify as an argument the fibonacci number you wish to calculate\n");
		return 1;
	}

	long n = strtol( argv[1], NULL, 10 );
	if( n < 0 ) {
		fprintf(stderr, "Must be greater than or equal to 0.\n");
		return 1;
	}

	mutex_t mutex; init_MUTEX(&mutex);
	condition_t cond; init_CONDITION(&cond);

	// Spin up the job systems
	init_JOBS();

	const int sampleSize = 10;
	usec_t totaltime = 0;
	for( int i=0; i<sampleSize; i++ ) {

		unsigned long long fib_n; 
		fibonacci_job_params_t params = { n };

		usec_t timebase = microseconds();
		jobid job = queue_JOB( NULL, n, cpuBound, &fib_n, (jobfunc_f)fibonacci, &params);
				
		lock_MUTEX(&mutex);
		while( join_deadline_JOB( (uint32)n, &mutex, &cond ) < 0 );
		unlock_MUTEX(&mutex);
		
		usec_t jobend = microseconds();
		usec_t elapsed = jobend - timebase;
		printf("The %lluth fibonacci number is %lld (job completed in %5.2f sec)\n", (unsigned long long)n, fib_n, (double)elapsed / usec_perSecond);

		totaltime += elapsed;
	}

	shutdown_JOBS();

	printf("\n");
	printf("Total time:     %5.2f sec\n", (double)totaltime / usec_perSecond);
	printf("Avg   time:     %5.2f sec\n", (double)(totaltime / sampleSize) / usec_perSecond);
	return 0;
}

#endif
