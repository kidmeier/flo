#include <assert.h>

#include "control.maybe.h"
#include "control.swap.h"
#include "core.log.h"
#include "data.handle.h"
#include "data.list.h"
#include "job.control.h"
#include "job.core.h"
#include "job.fibre.h"
#include "job.histogram.h"
#include "job.queue.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.spinlock.h"
#include "sync.thread.h"
#include "time.core.h"

#include "core.system.h"

struct job_worker_s {

	short      id;
	thread_t   thread;

};

// Pthread workers ////////////////////////////////////////////////////////////

static bool job_queue_running = false;
static int schedule_work( struct job_worker_s* self ) {

	region_p pool = region( "job.core::schedule_work" );

	if( 0 > init_Job_queue_thread(self) )
		fatal0("init_Job_queue_thread(self) < 0");

	List *running = new_List( pool, sizeof(Job) );
	List *expired = new_List( pool, sizeof(Job) );

	while( job_queue_running ) {

		// Check for new work; wait for up to 1 sec if we lack existing work
		Job* job = dequeue_Job( isempty_List(running)
		                        ? usec_perSecond 
		                        : 0 );
		if( job ) {
			
			// All jobs should be waiting when they come off the front queue
			assert( jobWaiting == job->status );

			// Insert it into the runqueue at the appropriate place
			Job* insert_pt = NULL;
			find__List( running, insert_pt, job->deadline < insert_pt->deadline );
			insert_before_List( running, insert_pt, job );

		} 

		// Run a timeslice
		while( !isempty_List(running) ) {

			// Take first job from runqueue
			job = pop_front_List( running );

			assert( jobWaiting == job->status );

			// If the job is cancelled we set its continuation to the beginning
			// of the cleanup block (if any)
			if( job->cancelled ) {

				debug( "Job %p:%x.%u cancelled", job, job->id, job->deadline );
				job->fibre = (duff_t)-1;

			}

			job->status = jobRunning;
			jobstatus_e ret = job->run(job, job->result_p, job->params, &job->locals);

			// Can't be reborn without first dying...
			assert( jobNew != ret );

			// Force the job to run if its been cancelled; the next time it is 
			// schedule the cleanup block will run and then it will terminate.
			if( job->cancelled ) {
				if( jobExited != ret && jobDone != ret  )
					ret = jobRunning;
			}

			// Implement state transition
			switch( ret ) {
			case jobRunning: { // job yielded to allow higher prio processes to go

				Job *insert_pt;

				job->status = jobWaiting;
				find__List( running, insert_pt, job->deadline < insert_pt->deadline );
				insert_before_List( running, insert_pt, job );
				break;
			}
			case jobBlocked: // job is blocked on a waitqueue; release our lock
				trace( "Job %p:%x.%u is blocked", job, job->id, job->deadline );
				unlock_SPINLOCK( &job->lock );
				break;

			case jobWaiting:   // the job is polling a condition; expire it
			case jobYielded: { // or has relinquished its run status

				Job *insert_pt;

				trace( "Job %p:%x.%u is waiting", job, job->id, job->deadline );

				job->status = jobWaiting;
				find__List( expired, insert_pt, job->deadline < insert_pt->deadline );
				insert_before_List( expired, insert_pt, job );
				break;
			}
			case jobExited: // the thread called exit_job(); notify and free
			case jobDone:   // the thread function finished; notify and free
				
				debug( "Job %p:%x.%u completed", job, job->id, job->deadline );
				
				job->status = jobDone;

				upd_Job_histogram( job->deadline, -1 );
				wakeup_waitqueue_Job( &job->waitqueue_lock, &job->waitqueue );
				free_Job( job );
				break;
			// This is bad, die.
			default:
				fatal( "Invalid job status: %d", ret );
				break;
			}

		}

		// Move on to next timeslice
		swap( List*, running, expired );

	}

	rfree( pool );
	return 0;

}

// Public API /////////////////////////////////////////////////////////////////

static int                  n_workers;
static struct job_worker_s* workers;

int   init_Jobs( int n_workers ) {

	if( init_Job_queue() < 0 )
		return -1;
	job_queue_running = true;

	workers = calloc( n_workers, sizeof(struct job_worker_s) );

	for( int i=0; i<n_workers; i++ ) {

		workers[i].id = i;

		int ret = create_THREAD( &workers[i].thread, 
		                         (threadfunc_f)schedule_work, 
		                         &workers[i] );
		if( ret < 0 ) {
			job_queue_running = false;
			for( int j=0; j<i; j++ ) 
				join_THREAD( &workers[j].thread, NULL );
			return ret;
		}

	}

	return 0;

}

void             shutdown_Jobs(void) {

	job_queue_running = false;

	for( int i=0; i<n_workers; i++ ) {
		join_THREAD( &workers[i].thread, NULL );
	}

}

Handle submit_Job( uint32     deadline, 
                   jobclass_e jobclass, 
                   pointer    result_p, 
                   jobfunc_f  run, 
                   pointer    params ) {
	
	return call_Job( NULL, deadline, jobclass, result_p, run, params );

}

Handle   call_Job( Job*       parent, 
                   uint32     deadline, 
                   jobclass_e jobclass, 
                   pointer    result_p, 
                   jobfunc_f  run, 
                   pointer    params ) {

	Handle id = alloc_Job( deadline, jobclass, result_p, run, params );
	Job* job = deref_Handle(Job,id);

	debug( "Job %p:%x.%u submitted", job, job->id, job->deadline );

	if( parent )
		sleep_waitqueue_Job( &job->waitqueue_lock, &job->waitqueue, parent );
	
	upd_Job_histogram( job->deadline, 1 );

	insert_Job( job );
	unlock_SPINLOCK( &job->lock );

	return id;

}

void         cancel_Job( Handle hdl ) {

	if( !isvalid_Handle(hdl) )
		return;

	Job *job = deref_Handle(Job, hdl);
	job->cancelled = true;

	if( 0 == trylock_SPINLOCK( &job->lock ) ) {

		if( jobBlocked == job->status ) {
				
			job->status = jobCancelled;
			insert_Job( job );
			
		}
		unlock_SPINLOCK( &job->lock );

	}

}
int   join_deadline_Job( uint32 deadline, mutex_t* mutex, condition_t* signal ) {

	return wait_Job_histogram(deadline, mutex, signal);

}

#ifdef __job_core_TEST__

#include <stdlib.h>
#include "job.control.h"

declare_job( unsigned long long, fibonacci,
             unsigned long long n );

define_job( unsigned long long, fibonacci,

            unsigned long long n_1;
            unsigned long long n_2;

            Handle job_n_1;
            Handle job_n_2 )
{
	begin_job;

	if( 0 == arg( n ) ) {

		exit_job( 0 );

	} else if( 1 == arg( n ) ) {

		exit_job( 1 );

	}

	call_job( local(job_n_1), 
	          arg(n) - 1, 
	          rand() % maxJobClass, 
	          &local(n_1), 
	          fibonacci, 
	          arg(n) - 1 );
	call_job( local(job_n_2), 
	          arg(n) - 2, 
	          rand() % maxJobClass, 
	          &local(n_2), 
	          fibonacci, 
	          arg(n) - 2 );
		
	exit_job( local(n_1) + local(n_2) );

	end_job;
}

int main( int argc, char* argv[] ) {

	if( argc < 3 ) {
		fprintf(stderr, "usage: %s <n_workers> <nth fibonacci # to calculate>\n", argv[0]);
		return 1;
	}

	int n_threads = (int)strtol( argv[1], NULL, 10 );
	
	long n = strtol( argv[2], NULL, 10 );
	if( n < 0 ) {
		fprintf(stderr, "Must be greater than or equal to 0.\n");
		return 1;
	}
	printf("sizeof(jobclass_e) = %zu\n", sizeof(jobclass_e));
	printf("sizeof(jobstatus_e) = %zu\n", sizeof(jobstatus_e));
	printf("sizeof(fibre_t) = %zu\n", sizeof(fibre_t));

	printf("sizeof(Job) = %zu\n", sizeof(Job));
	printf("sizeof(Handle) = %zu\n", sizeof(Handle));

	mutex_t mutex; init_MUTEX(&mutex);
	condition_t cond; init_CONDITION(&cond);

	// Spin up the job systems
	init_Jobs( n_threads );

	const int sampleSize = 10;
	usec_t totaltime = 0;
	for( int i=0; i<sampleSize; i++ ) {

		unsigned long long fib_n; 
		typeof_Job_params(fibonacci) params = { n };

		usec_t timebase = microseconds();
		submit_Job( n, cpuBound, &fib_n, (jobfunc_f)fibonacci, &params);
				
		lock_MUTEX(&mutex);
		while( join_deadline_Job( (uint32)n, &mutex, &cond ) < 0 );
		unlock_MUTEX(&mutex);
		
		usec_t jobend = microseconds();
		usec_t elapsed = jobend - timebase;
		printf("The %lluth fibonacci number is %lld (job completed in %5.2f sec)\n", (unsigned long long)n, fib_n, (double)elapsed / usec_perSecond);

		totaltime += elapsed;
	}

	shutdown_Jobs();

	printf("\n");
	printf("Total time:     %5.2f sec\n", (double)totaltime / usec_perSecond);
	printf("Avg   time:     %5.2f sec\n", (double)(totaltime / sampleSize) / usec_perSecond);
	return 0;
}

#endif
