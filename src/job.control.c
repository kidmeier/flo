#ifdef __job_control_TEST__

#include "job.channel.h"
#include "job.control.h"
#include "core.alloc.h"

declare_job( unsigned long long, fibonacci,
             unsigned long long n );

declare_job( int, fib_producer,

             unsigned N;
             job_channel_p out );

declare_job( int, fib_consumer,

             unsigned N;
             job_channel_p in );

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

	spawn_job( local(job_n_1), arg(n) - 1, cpuBound, &local(n_1), fibonacci, { arg(n) - 1 } );
	spawn_job( local(job_n_2), arg(n) - 2, cpuBound, &local(n_2), fibonacci, { arg(n) - 2 } );
		
	exit_job( local(n_1) + local(n_2) );

	end_job;
}

define_job( int, fib_consumer,
            
            unsigned i;
            unsigned long long fib ) {

	begin_job;

	for( local(i) = 0; local(i)<arg(N); local(i)++ ) {

		readch( arg(in), local(fib) );
		printf("fib(%d) = %llu\n", local(i), local(fib));
		
	}

	end_job;
}

define_job( int, fib_producer,

            unsigned i;

            jobid fib_i;
            unsigned long long fib ) {

	begin_job;

	for( local(i)=0; local(i)<arg(N); local(i)++ ) {

		spawn_job( local(fib_i), (uint32)local(i), cpuBound, &local(fib), fibonacci, { local(i) } );
		writech( arg(out), local(fib) );

	}

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

	printf("\n");
	printf("This test computes the first N fibonacci numbers. Fibonacci numbers are \n");
	printf("brute-force computed recursively; each recursive call spawns a new job. \n");
	printf("The results are written to a channel by a producer job and consumed by  \n");
	printf("a consuming job; channels are blocking in the style of Hoare's CSP.     \n");
	printf("\n");

	init_JOBS();

	// Make the channel size not a multiple of sizeof(int)
	// this ensures we exercise more code paths.
	job_channel_p chan = new_CHAN( sizeof(char), 13 );

	int c_ret;
	fib_consumer_job_params_t c_params = { n, chan };

	int p_ret;
	fib_producer_job_params_t p_params = { n, chan };

	usec_t begin = microseconds();

	jobid cons = submit_JOB( nullJob, 0, ioBound, &c_ret, (jobfunc_f)fib_consumer, &c_params);
	jobid prod = submit_JOB( nullJob, 0, ioBound, &p_ret, (jobfunc_f)fib_producer, &p_params);

	mutex_t mutex; init_MUTEX(&mutex);
	condition_t cond; init_CONDITION(&cond);

	lock_MUTEX(&mutex);
	while( join_deadline_JOB( 0, &mutex, &cond ) < 0 );
	unlock_MUTEX(&mutex);

	usec_t end = microseconds();
	usec_t elapsed = end - begin;
	printf("\nTotal time:     %5.2f sec\n", (double)elapsed / usec_perSecond);

	shutdown_JOBS();
	return 0;
}

#endif
