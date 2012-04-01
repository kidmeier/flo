#ifdef __job_control_TEST__

#include <stdio.h>

#include "job.channel.h"
#include "job.control.h"

declare_job( unsigned long long, fibonacci,
             unsigned long long n );

declare_job( int, fib_producer,

             unsigned N;
             Channel* out );

declare_job( int, fib_consumer,

             unsigned N;
             Channel* in );

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

	call_job( local(job_n_1), arg(n) - 1, cpuBound, &local(n_1), fibonacci, arg(n) - 1 );
	call_job( local(job_n_2), arg(n) - 2, cpuBound, &local(n_2), fibonacci, arg(n) - 2 );
		
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

            Handle fib_i;
            unsigned long long fib ) {

	begin_job;

	for( local(i)=0; local(i)<arg(N); local(i)++ ) {

		call_job( local(fib_i), (uint32)local(i), cpuBound, &local(fib), fibonacci, local(i) );
		writech( arg(out), local(fib) );

	}
	flushch( arg(out) );

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

	printf("\n");
	printf("This test computes the first N fibonacci numbers. Fibonacci numbers are \n");
	printf("brute-force computed recursively; each recursive call spawns a new job. \n");
	printf("The results are written to a channel by a producer job and consumed by  \n");
	printf("a consuming job; channels are blocking in the style of Hoare's CSP.     \n");
	printf("\n");

	init_Jobs( n_threads );

	// Make the channel size not a multiple of sizeof(int)
	// this ensures we exercise more code paths.
	Channel* chan = new_Channel( sizeof(char), 13 );

	int c_ret;
	typeof_Job_params(fib_consumer) c_params = { n, chan };

	int p_ret;
	typeof_Job_params(fib_producer) p_params = { n, chan };

	usec_t begin = microseconds();

	Handle cons = submit_Job( 0, ioBound, &c_ret, (jobfunc_f)fib_consumer, &c_params);
	Handle prod = submit_Job( 0, ioBound, &p_ret, (jobfunc_f)fib_producer, &p_params);

	mutex_t mutex; init_MUTEX(&mutex);
	condition_t cond; init_CONDITION(&cond);

	lock_MUTEX(&mutex);
	while( join_deadline_Job( 0, &mutex, &cond ) < 0 );
	unlock_MUTEX(&mutex);

	usec_t end = microseconds();
	usec_t elapsed = end - begin;
	printf("\nTotal time:     %5.2f sec\n", (double)elapsed / usec_perSecond);

	shutdown_Jobs();
	return 0;
}

#endif
