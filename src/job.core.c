#include "control.maybe.h"
#include "data.list.h"
#include "job.core.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.spinlock.h"
#include "sync.thread.h"
#include "time.core.h"

#include "core.alloc.h"
#include "core.system.h"

struct job_queue_s {

	uint32           id;
	struct pt        pthr;

	uint32           deadline;
	jobclass_e  jobclass;

	void*            result_p;
	jobfunc_f        run;
	void*            params;
	void*            locals;

	jobstatus_e status;

	struct job_queue_s* parent;

	llist_mixin( struct job_queue_s );

};

struct histogram_s {

	uint32 deadline;
	int    count;

	mutex_t*     mutex;
	condition_t* signal;

	llist_mixin( struct job_queue_s );

};

// Histogram //////////////////////////////////////////////////////////////////
// NOTE: These functions are not implicitly thread safe. It is up to the caller
//       to ensure calls to these functions are performed serially across
//       threads. In other words, callers should lock on a global object
//       before calling these.

static struct histogram_s* deadline_histogram = NULL;
static struct histogram_s* free_histogram_list = NULL;

static spinlock_t          free_histogram_lock;

static struct histogram_s* alloc_histogram( uint32 deadline ) {

	struct histogram_s* hist = NULL;

	lock_SPINLOCK( &free_histogram_lock );
	if( NULL != free_histogram_list ) {

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

	llist_init( hist );

	return hist;

}

static void insert_histogram( struct histogram_s* hist ) {

	if( !deadline_histogram ) {

		deadline_histogram = hist;
		return;

	}

	struct histogram_s* node = NULL;
	llist_find( deadline_histogram, node, node->deadline == hist->deadline );
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
	if( !node ) {
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

		unlock_SPINLOCK( &free_histogram_lock );

		// If histogram is empty then return error
		if( !deadline_histogram )
			return -1;

		// If we are asking to wait on a deadline that precedes the first
		// in our histogram, we assume that all jobs in that deadline have
		// completed and the histogram has been freed
		if( deadline < deadline_histogram->deadline ) {
			return 0;
		}

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
static job_queue_p free_job_list = NULL;
static spinlock_t  free_job_lock;

static job_queue_p job_queue = NULL;
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
	PT_INIT( &job->pthr );
	
	job->deadline = deadline;
	job->jobclass = jobclass;

	job->result_p = result_p;
	job->run = run;
	job->params = params;
	job->locals = NULL;

	job->status = jobWaiting;

	job->parent = parent;

	llist_init( job );

	return job->id;

}

static void  insert_job( job_queue_p job ) {
	
	lock_SPINLOCK( &job_queue_lock );

	// Update the histogram
	upd_histogram( job->deadline, 1 );

	// Empty queue
	if( !job_queue ) {
		// Interleave the lock so that if someone enters queue_wait
		// at the same time we get here, we make sure they receive 
		// the broadcast
		lock_MUTEX( &job_queue_mutex );

		job_queue = job;

		unlock_SPINLOCK( &job_queue_lock );

		// Signal anyone waiting for jobs
		broadcast_CONDITION( &job_queue_signal );
		unlock_MUTEX( &job_queue_mutex );

		return;
	}

	job_queue_p node;

	llist_find( job_queue, node, job->deadline < node->deadline );
	llist_insert_at( job_queue, node, job );

	unlock_SPINLOCK( &job_queue_lock );

}

static jobid alloc_job( job_queue_p parent, uint32 deadline, jobclass_e jobclass, void* result_p, jobfunc_f run, void* params ) {

	job_queue_p job = NULL;

	// Resurrect a job from the completed list
	lock_SPINLOCK( &free_job_lock );

	if( free_job_list ) {

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

	job->parent = NULL;

	lock_SPINLOCK( &free_job_lock );

	  llist_push_front( free_job_list, job );

	unlock_SPINLOCK( &free_job_lock );

}

static job_queue_p dequeue_job( void ) {

	job_queue_p job = NULL;

	lock_SPINLOCK( &job_queue_lock );

	llist_pop_front( job_queue, job );

	unlock_SPINLOCK( &job_queue_lock );
	return job;
}

static void queue_wait( uint64 usec ) {

	lock_SPINLOCK( &job_queue_lock );

	if( NULL != job_queue ) {
		unlock_SPINLOCK( &job_queue_lock );
		return;
	}

	lock_MUTEX( &job_queue_mutex );
	// Interleave the lock so that no one can change job_queue between the time
	// we check NULL above and here
	  unlock_SPINLOCK( &job_queue_lock );
	  timed_wait_CONDITION( usec, &job_queue_signal, &job_queue_mutex );
	unlock_MUTEX( &job_queue_mutex );

}

// Pthread workers ////////////////////////////////////////////////////////////

static bool job_queue_running = false;

static int schedule_work( const char* tid ) {

	job_queue_p runqueue = NULL;

//	fprintf(stderr, "[%s] Starting up\n", tid);
	while( job_queue_running ) {

		// Ask for some work
		job_queue_p job = dequeue_job();

		if( job ) {

//			fprintf(stderr, "[%s] Assigned job     0x%x: id=0x%x, deadline=%d, arg=0x%x\n", tid,
//			        job, job->id, job->deadline, job->arg);

			// Insert it into the runqueue at the appropriate place
			job_queue_p node = NULL;
			llist_find( runqueue, node, job->deadline < node->deadline );
			llist_insert_at( runqueue, node, job );
			
		} else if( !runqueue ) {

			// No work to steal and we don't have any in our bucket, wait
			queue_wait( usec_perSecond ); 
			continue;

		}

		// Do the work
		job = runqueue;
		while( job ) {

//			fprintf(stderr, "[%s] About to run job 0x%x: id=0x%x, deadline=%d, arg=0x%x\n", tid,
//			        job, job->id, job->deadline, job->arg);

			job->status = jobRunning;
			int running = PT_SCHEDULE( job->run(job, job->result_p, job->params, &job->locals) );
		
			if( !running )  {

				job_queue_p finished_job = job;
				job_queue_p next_job = job->next;

				finished_job->status = jobDone;

//				fprintf(stderr, "[%s] Job finished     0x%x: id=0x%x, deadline=%d, arg=0x%x\n", tid,
//				        job, job->id, job->deadline, job->arg);

				// Remove from the run list and put it back into the pool
				llist_remove( runqueue, finished_job );
				free_job( finished_job );				

				job = next_job;

			} else {

				job->status = jobWaiting;
				job = job->next;

			}

		}
		
	}

}

// Public API /////////////////////////////////////////////////////////////////

static int       n_worker_threads;
static thread_t* worker_threads;

int   init_JOBS(void) {

	if( init_job_queue() < 0 )
		return -1;
	job_queue_running = true;

	n_worker_threads = cpu_count_SYS();
	worker_threads = new_array( NULL, thread_t, n_worker_threads );

	for( int i=0; i<n_worker_threads; i++ ) {

		int ret = create_THREAD( &worker_threads[i], (threadfunc_f)schedule_work, (void*)i );
		if( ret < 0 ) {
			for( int j=0; j<i; j++ ) 
				cancel_THREAD( &worker_threads[j] );
			return ret;
		}

	}

	return 0;

}

void             shutdown_JOBS(void) {

	job_queue_running = false;

	for( int i=0; i<n_worker_threads; i++ ) {
		join_THREAD( &worker_threads[i], NULL );
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

	spawn_job( local(job_n_1), arg(n) - 2, cpuBound, &local(n_1), fibonacci, { arg(n) - 1 } );
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
	
	unsigned long long fib_n; 
	fibonacci_job_params_t params = { n };
	jobid job = queue_JOB( NULL, n, cpuBound, &fib_n, (jobfunc_f)fibonacci, &params);
	                       
	
	fprintf(stderr, "Job submitted, waiting results...\n");
	lock_MUTEX(&mutex);
	join_deadline_JOB( (uint32)n, &mutex, &cond );
	unlock_MUTEX(&mutex);

	printf("The %lluth fibonacci number is %lld\n", (unsigned long long)n, fib_n );

	shutdown_JOBS();
	return 0;
}

#endif
