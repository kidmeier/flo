#include "control.maybe.h"
#include "control.swap.h"
#include "core.log.h"
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

#include "core.alloc.h"
#include "core.system.h"

struct job_worker_s {

	short      id;
	thread_t   thread;

};

// Pthread workers ////////////////////////////////////////////////////////////

static bool job_queue_running = false;
static int schedule_work( struct job_worker_s* self ) {

//	int N = self->id;

	llist(Job, running);
	llist(Job, expired);

	while( job_queue_running ) {

		// Check for new work; wait for up to 1 sec if we lack existing work
		Job* job = dequeue_Job( llist_isempty(running) 
		                               ? usec_perSecond 
		                               : 0 );
		if( job ) {

			// Insert it into the runqueue at the appropriate place
			Job* insert_pt = NULL;
			llist_find( running, insert_pt, job->deadline < insert_pt->deadline );
			llist_insert_at( running, insert_pt, job );
			
		} 

		// Run a timeslice
		while( !llist_isempty(running) ) {

			// Take first job from runqueue
			llist_pop_front( running, job );

			job->status = jobRunning;
			job->status = job->run(job, job->result_p, job->params, &job->locals);
			
			switch( job->status ) {
			case jobRunning: { // job yielded to allow higher prio processes to go

				Job* insert_pt;
				llist_find( running, insert_pt, job->deadline < insert_pt->deadline );
				llist_insert_at( running, insert_pt, job );
				break;
			}
			case jobBlocked: // job is blocked on some waitqueue; we are off the hook
//				trace( "BLOCKED 0x%x:%x", (unsigned)job, job->id );
				break;

			case jobWaiting: { // the thread is polling a condition; expire it

				Job* insert_pt;

//				trace( "WAITING 0x%x:%x", (unsigned)job, job->id );
				llist_find( expired, insert_pt, job->deadline < insert_pt->deadline );
				llist_insert_at( expired, insert_pt, job );
				break;
			}
			case jobExited: // the thread called exit_job(); notify and free
			case jobDone:   // the thread function completed; notify and free
				
//				trace( "COMPLETE 0x%x:%x", (unsigned)job, job->id );
				wakeup_waitqueue_Job( &job->waitqueue_lock, &job->waitqueue );
				free_Job( job );
				break;
			}

		}

		// Move on to next timeslice
		swap( Job*, running, expired );
	}
	return 0;

}

// Public API /////////////////////////////////////////////////////////////////

static int                  n_workers;
static struct job_worker_s* workers;

int   init_Jobs( int n_workers ) {

	if( init_Job_queue() < 0 )
		return -1;
	job_queue_running = true;

	workers = new_array( NULL, struct job_worker_s, n_workers );

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

jobid null_Job = { 0, NULL };

jobid submit_Job( uint32 deadline, jobclass_e jobclass, void* result_p, jobfunc_f run, void* params ) {

	jobid id = alloc_Job( deadline, jobclass, result_p, run, params );
	insert_Job( id.job );

	return id;

}

jobstatus_e status_Job( jobid jid ) {

	if( jid.id != jid.job->id )
		return jobDone;

	return jid.job->status;

}

int   join_deadline_Job( uint32 deadline, mutex_t* mutex, condition_t* signal ) {

	return wait_Job_histogram(deadline, mutex, signal);

}

#ifdef __job_core_TEST__

#include "job.control.h"

declare_job( unsigned long long, fibonacci,
             unsigned long long n );

define_job( unsigned long long, fibonacci,

            unsigned long long n_1;
            unsigned long long n_2;

            jobid job_n_1;
            jobid job_n_2 )
{
	begin_job;

	if( 0 == arg( n ) ) {

		exit_job( 0 );

	} else if( 1 == arg( n ) ) {

		exit_job( 1 );

	}

	spawn_job( local(job_n_1), arg(n) - 1, cpuBound, &local(n_1), fibonacci, arg(n) - 1 );
	spawn_job( local(job_n_2), arg(n) - 2, cpuBound, &local(n_2), fibonacci, arg(n) - 2 );
		
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
		jobid job = submit_Job( n, cpuBound, &fib_n, (jobfunc_f)fibonacci, &params);
				
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
