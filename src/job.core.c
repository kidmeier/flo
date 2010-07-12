#include "time.core.h"
#include "job.core.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.spinlock.h"
#include "sync.thread.h"

#include "core.alloc.h"
#include "core.system.h"

struct job_queue_s {

	uint32           id;
	struct pt        pthr;

	uint32           deadline;
	enum jobclass_e  jobclass;

	jobfunc_f        run;
	void*            arg;

	enum jobstatus_e status;

	struct job_queue_s* next;
	struct job_queue_s* prev;

};

struct histogram_s {

	uint32 deadline;
	int    count;

	mutex_t*     mutex;
	condition_t* signal;

	struct histogram_s* next;
	struct histogram_s* prev;

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

	if( free_histogram_list ) {

		hist = free_histogram_list;
		free_histogram_list = free_histogram_list->next;

	} else {

		hist = new( NULL, struct histogram_s );

	}

	hist->deadline = deadline;
	hist->count = 0;

	hist->mutex = NULL;
	hist->signal = NULL;
	hist->next = NULL;
	hist->prev = NULL;

	return hist;

}

static void insert_histogram( struct histogram_s* hist ) {

	if( !deadline_histogram ) {

		deadline_histogram = hist;
		return;

	}

	struct histogram_s* prev = NULL;
	struct histogram_s* node = deadline_histogram;
	while( node ) {

		if( hist->deadline == node->deadline )
			break;

		prev = node;
		node = node->next;

	}

	hist->prev = prev;
	hist->next = node;
	if( hist->prev )
		hist->prev->next = hist;
	if( hist->next )
		hist->next->prev = hist;

	if( deadline_histogram == node )
		deadline_histogram = hist;

}

static void free_histogram( struct histogram_s* hist ) {

	lock_SPINLOCK( &free_histogram_lock );

	// Notify if anyone is waiting on this
	if( hist->mutex && hist->signal ) {

		unlock_SPINLOCK( &free_histogram_lock );

		lock_MUTEX( hist->mutex );
		broadcast_CONDITION( hist->signal );
		unlock_MUTEX( hist->mutex );

	} else 
		unlock_SPINLOCK( &free_histogram_lock );
	
	if( hist->prev )
		hist->prev->next = hist->next;
	if( hist->next )
		hist->next->prev = hist->prev;

	hist->next = free_histogram_list;
	hist->prev = NULL;
	free_histogram_list = hist;

}

static struct histogram_s* find_histogram( uint32 deadline ) {

	// Find the histogram node
	struct histogram_s* node = deadline_histogram;
	while( node ) {
		if( node->deadline == deadline )
			break;

		node = node->next;
	}

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

		// If we are asking to wait on a deadline that precedes the first
		// in our histogram, we assume that all jobs in that deadline have
		// completed and the histogram has been freed
		if( deadline_histogram && deadline < deadline_histogram->deadline ) {
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

static job_queue_p job_queue;
static spinlock_t  job_queue_lock;

static mutex_t     job_queue_mutex;
static condition_t job_queue_signal;

static int init_job_queue(void) {

	init_SPINLOCK( &free_histogram_lock );

	init_SPINLOCK( &job_queue_lock );
	init_SPINLOCK( &free_job_lock );

	init_MUTEX( &job_queue_mutex );
	init_CONDITION( &job_queue_signal );

	return 0;
}

static uint32 alloc_id() {

	return (uint32)microseconds();

}

static uint32 init_job( job_queue_p job, uint32 deadline, enum jobclass_e jobclass, jobfunc_f run, void* arg ) {

	job->id = alloc_id();
	PT_INIT( &job->pthr );
	
	job->deadline = deadline;
	job->jobclass = jobclass;

	job->run = run;
	job->arg = arg;

	job->status = jobWaiting;

	job->prev = NULL;
	job->next = NULL;

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

	job_queue_p prev = NULL;
	job_queue_p node = job_queue;

	// Find the spot to insert
	while( node ) {

		if( job->deadline < node->deadline )
			break;
		
		prev = node;
		node = node->next;

	}

	job->prev = prev;
	job->next = node;
	if( job->prev )
		job->prev->next = job;
	if( job->next )
		job->next->prev = job;

	if( job_queue == node )
		job_queue = job;

	unlock_SPINLOCK( &job_queue_lock );

}

static jobid alloc_job( uint32 deadline, enum jobclass_e jobclass, jobfunc_f run, void* arg ) {

	job_queue_p job = NULL;

	// Resurrect a job from the completed list
	lock_SPINLOCK( &free_job_lock );

	if( free_job_list ) {

		job = free_job_list;
		free_job_list = free_job_list->next;		
		job->next = NULL;

		unlock_SPINLOCK( &free_job_lock );

	} else {

		unlock_SPINLOCK( &free_job_lock );

		// Need a whole new one
		if( !job_pool )
			job_pool = autofree_pool();

		job = new(job_pool, job_queue_t);

	}

	// Configure
	uint32 id = init_job( job, deadline, jobclass, run, arg );

	// Insert into the active list
	insert_job( job );

	// Return the jobid
	return (jobid) { id, job };
}

static void free_job( job_queue_p job ) {

	lock_SPINLOCK( &free_job_lock );

	upd_histogram( job->deadline, -1 );

	if( job->prev )
		job->prev->next = job->next;
	if( job->next )
		job->next->prev = job->prev;
	job->next = free_job_list;
	job->prev = NULL;
	free_job_list = job;

	unlock_SPINLOCK( &free_job_lock );

}

static job_queue_p dequeue_job( void ) {

	job_queue_p job = NULL;

	lock_SPINLOCK( &job_queue_lock );

	if( job_queue ) {

		job = job_queue;
		job_queue = job_queue->next;
		if( job_queue )
			job_queue->prev = NULL;

		job->prev = NULL;
		job->next = NULL;

	}

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
	// Interleave the lock so that no one can job_queue between the time
	// we check NULL above and here
	  unlock_SPINLOCK( &job_queue_lock );
	  timed_wait_CONDITION( usec, &job_queue_signal, &job_queue_mutex );
	unlock_MUTEX( &job_queue_mutex );

}

// Pthread workers ////////////////////////////////////////////////////////////

static bool job_queue_running = false;

static int schedule_work( int N ) {

	job_queue_p runqueue = NULL;

//	fprintf(stderr, "[%d] Starting up\n", N);
	while( job_queue_running ) {

		// Ask for some work
		job_queue_p job = dequeue_job();

		if( job ) {

//			fprintf(stderr, "[%d] Assigned job     0x%x: id=0x%x, deadline=%d, arg=0x%x\n", N,
//			        job, job->id, job->deadline, job->arg);

			// Insert it into the runqueue at the appropriate place
			job_queue_p prev = NULL;
			job_queue_p node = runqueue;
			while( node ) {

				if( job->deadline < node->deadline ) 
					break;

				prev = node;
				node = node->next;
			}

			
			job->prev = prev;
			job->next = node;
			if( job->prev )
				job->prev->next = job;
			if( job->next )
				job->next->prev = job;

			if( runqueue == node )
				runqueue = job;
			
		} else if( !runqueue ) {

			// No work to steal and we don't have any in our bucket, wait
			queue_wait( 1000000ULL ); 
			continue;

		}

		// Do the work
		job = runqueue;
		while( job ) {

//			fprintf(stderr, "[%d] About to run job 0x%x: id=0x%x, deadline=%d, arg=0x%x\n", N,
//			        job, job->id, job->deadline, job->arg);

			job->status = jobRunning;
			int running = PT_SCHEDULE( job->run(&job->pthr, job->arg) );
		
			if( !running )  {

				job_queue_p finished_job = job;
				finished_job->status = jobDone;

//				fprintf(stderr, "[%d] Job finished     0x%x: id=0x%x, deadline=%d, arg=0x%x\n", N,
//				        job, job->id, job->deadline, job->arg);

				// Remove from the run list and put it back into the pool
				if( job->prev )
					job->prev->next = job->next;
				if( job->next )
					job->next->prev = job->prev;

				job = job->next;

				if( runqueue == finished_job )
					runqueue = job;

				finished_job->next = NULL;
				finished_job->prev = NULL;
				free_job( finished_job );				

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


jobid queue_JOB( uint32 deadline, enum jobclass_e jobclass, jobfunc_f run, void* arg ) {

	jobid job = alloc_job( deadline, jobclass, run, arg  );
	return job;

}

enum jobstatus_e status_JOB( jobid jid ) {

	if( jid.id != jid.job->id )
		return jobDone;

	return jid.job->status;

}

int   join_deadline_JOB( uint32 deadline, mutex_t* mutex, condition_t* signal ) {

	return wait_histogram(deadline, mutex, signal);

}

#ifdef __job_core_TEST__

typedef struct fib_s {

	jobid              job;
	unsigned long long n;
	unsigned long long result;

	struct fib_s* n1;
	struct fib_s* n2;

} fib_t;

defjob( fibonacci, fib_t* op ) {
	begin_job;

	if( 0 == op->n  ) {
		op->result = 0;
		exit_job;
	} else if( 1 == op->n ) {
		op->result = 1;
		exit_job;
	}

	op->n1 = (fib_t*)malloc(sizeof(fib_t));
	op->n2 = (fib_t*)malloc(sizeof(fib_t));
	*op->n1 = (fib_t){ .n = op->n - 1, .result = -1, .n1 = NULL, .n2 = NULL };
	*op->n2 = (fib_t){ .n = op->n - 2, .result = -1, .n1 = NULL, .n2 = NULL };

	spawn_job( op->n1->job, op->n1->n, cpuBound, (jobfunc_f)fibonacci, op->n1 );
	spawn_job( op->n2->job, op->n2->n, cpuBound, (jobfunc_f)fibonacci, op->n2 );
		
	op->result = op->n1->result + op->n2->result;
	free(op->n1); free(op->n2);

	exit_job;

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
	
	fib_t fib = { .n = n, .result = -1 };
	jobid job = queue_JOB( fib.n, cpuBound, (jobfunc_f)fibonacci, &fib );
	
	fprintf(stderr, "Job submitted, waiting results...\n");
	lock_MUTEX(&mutex);
	join_deadline_JOB( (uint32)fib.n, &mutex, &cond );
	unlock_MUTEX(&mutex);

	printf("The %lldth fibonacci number is %lld\n", fib.n, fib.result );

	shutdown_JOBS();
	return 0;
}

#endif