#ifndef __job_control_h__
#define __job_control_h__

#include "job.core.h"
#include "job.fibre.h"
#include "job.queue.h"

// Job control ////////////////////////////////////////////////////////////////
//
// The macros below define a DSL job language. There is support for launching
// child jobs, waiting on existing jobs, etc. This is all implemented on top
// of "fibres", a stackless thread implemented using Duff's device.
//
// Since there is no stack, thread local state must be allocated on the heap.
// The job control DSL provides for declaring a set of local vars and 
// parameters which are macro-logically bundled into struct's and auto-allocated
// from the heap at job creation. Access to locals and parameters must occur 
// through accessor macros (see `arg` and `local`)
//
// ////////////////////////////////////////////////////////////////////////////

#define typeof_Job_params( job_name )	  \
	job_name##_job_params_t

#define typeof_Job_locals( job_name )	  \
	job_name##_job_locals_t

#define structof_Job_locals( job_name )	  \
	struct job_name##_job_locals_s

// Corresponds to function prototype, except it is NOT optional. This is where
// you can declare parameters to the job.
//
// To make your job callable from another compilation unit you can place this in
// a header to be included by consumers of your job.
//
// @name       - C identifier; declares C function that implements the job
// @returntype - C type expression specifying return type
// @params     - C-struct body (braces are supplied, semi-colon separated list); 
//                 lists named parameters
#define declare_job( returntype, name, params )	  \
	typedef struct { \
		params ; \
	} typeof_Job_params( name ); \
	typedef structof_Job_locals(name) typeof_Job_locals(name) ; \
	jobstatus_e name ( Job*, returntype *, typeof_Job_params(name) *, typeof_Job_locals(name) ** )

// Corresponds to an actual function definition. All calls to this macro must 
// have a preceding call to declare_job in the same compilation unit with the 
// same @name and @returntype.
//
// @name       - C-identifier; defines C function that implements the job
// @returntype - C-type expression specifying the return type
// @locals     - Brace-enclosed C-struct body; lists local vars needed by job
#define define_job( returntype, name, locals )	  \
	structof_Job_locals(name) { \
		locals ; \
	}; \
	jobstatus_e name ( Job                     * self, \
	                   returntype              * result, \
	                   typeof_Job_params(name) * _job_params, \
	                   typeof_Job_locals(name)** _job_locals ) { \
	if( !(*_job_locals) ) (*_job_locals) = ralloc( self->R, sizeof( typeof_Job_locals(name) ));

// Declares the beginning of a job definition. Must be the first statement 
// in the body of define_job
#define begin_job	  \
	begin_fibre( &self->fibre ) 

// Marks the end of a job definition. Must be the last statement 
// in the body of define_job
#define end_job \
		end_fibre( &self->fibre ) \
	}

// Corresponds to `return val;`. DO NOT use return. @val can be any legal C 
// expression, including a reference to a local, or named parameter, or an 
// expression involving any combination thereof.
//
// @val - C-expression
#define exit_job( val )	  \
	do { \
		if( result ) *result = (val); \
		exit_fibre( &self->fibre ); \
	} while(0)

// Yield this job; when it is rescheduled it will resume execution at the place
// of the @begin_job statement.
#define restart_job \
	restart_fibre( &self->fibre )

// Locals and parameters //////////////////////////////////////////////////////

// Make reference to a local variable. @name must have been defined in the 
// @locals block of the enclosing @define_job. Can be used anywhere a struct-
// member ref can be made:
//
//  - as an lvalue 
//  - as an rvalue
//  - can take its address
//
// @name - C-identifier; name of local variable to refer to
#define local( name ) \
	(*_job_locals)->name

// Make reference to a named parameter. @name must have been defined in the 
// @params block of the enclosing @declare_job. Can be used anywhere a struct-
// member ref can be made:
//
//  - as an lvalue 
//  - as an rvalue
//  - can take its address
//
// @name - C-identifier; name of local variable to refer to
#define arg( name ) \
	(_job_params)->name

// Job choreography ///////////////////////////////////////////////////////////

// Yields this job as long as @cond, when coerced to a boolean evaluates 
// to logical false.
//
// @cond - C-expression
#define wait_until( cond )	  \
	busywait_until( &self->fibre, (cond), jobWaiting )

// Yields this job as long as @cond, when coerced to a boolean evaluates 
// to logical true.
//
// @cond - C-expression
#define wait_while( cond ) \
	busywait_while( &self->fibre, (cond) )

// Puts this job to sleep until the job referred to by @jid completes.
//
// jid - expression of type jobid
#define wait_job( jid )	  \
	do { \
		lock_SPINLOCK( &(jid).job->waitqueue_lock ); \
		if( (jid).id == (jid).job->id \
		    && (jid).job->status < jobExited ) { \
			self->status = jobBlocked; \
			set_duff( &self->fibre ); \
			if( jobBlocked == self->status ) { \
				sleep_waitqueue_Job( NULL, &(jid).job->waitqueue, self ); \
				unlock_SPINLOCK( &(jid).job->waitqueue_lock ); \
				yield( jobBlocked ); \
			} \
		} else \
			unlock_SPINLOCK( &(jid).job->waitqueue_lock ); \
	} while(0)

// Yields this job until the job referred to by @jid completes.
//
// jid - expression of type jobid
#define busywait_job( jid ) \
	busywait_until( &self->fibre, \
	                jobRunning < (jid).job->run( (jid).job, \
	                                             &(jid).job->result_p, \
	                                             (jid).job->params, \
	                                             &(jid).job->locals ), \
	                jobWaiting)

// Puts calling job to sleep to be woken up `usec` from now
//
// @usec - number of microseconds (from time of call) until job is woken up
#define set_alarm( alarm, usec )	  \
	do { \
		set_alarm_Job( &(alarm), (usec) ); \
		set_duff( &self->fibre ); \
		if( alarmExpired != wait_alarm_Job( self, &(alarm) ) ) \
			yield( jobBlocked ); \
	} while(0)

// Launch a new child job but don't wait for it to complete
//
// @jid      - jobid; stores the jobid of the child job
// @deadline - uint32; set the deadline of the child job
// @jobclass - jobclass_e; job scheduling hint
// @result_p - pointer to job result; can be NULL
// @params   - pointer to job parameters struct; remains owned by caller
// @args     - C-struct initializer; initialize job parameters
#define submit_job( jid, deadline, jobclass, result_p, jobfunc, params ) \
	do { \
		(jid) = submit_Job( (deadline), (jobclass), (result_p), (jobfunc_f)(jobfunc), params ); \
	} while(0)

// Launch a new child job and yield this job until it completes.
//
// @jid      - jobid; stores the jobid of the child job
// @deadline - uint32; indicate the deadline of this job
// @jobclass - jobclass_e;
// @result_p - pointer to memory to hold job result; can be NULL
// @jobfunc  - name declared via @declare_job in same compilation unit
// @args     - C-struct initializer; initializes job parameters
#define spawn_job( jid, deadline, jobclass, result_p, jobfunc, args... ) \
	do { \
		typeof_Job_params(jobfunc) * params = ralloc(self->R, sizeof( typeof_Job_params(jobfunc) )); \
		*(params) = (typeof_Job_params(jobfunc)){ args }; \
		submit_job( jid, deadline, jobclass, result_p, jobfunc, params ); \
		wait_job( jid ); \
	} while(0)

// Yield this job to allow other(s) to run.
#define yield( status )	  \
	yield_fibre( &self->fibre, (status) )

// Wakeup any jobs waiting on this job's runqueue
#define notify( jid ) \
	wakeup_waitqueue_Job( &(jid).job->waitqueue_lock, \
	                      &(jid).job )

// Put job to sleep on its own wait queue until someone wakes it up
#define wait \
	do { \
		lock_SPINLOCK( &self->waitqueue_lock ); \
		self->status = jobBlocked; \
		set_duff( &self->fibre ); \
		if( jobBlocked == self->status ) { \
			sleep_waitqueue_Job( NULL, &self->waitqueue, self ); \
			unlock_SPINLOCK( &self->waitqueue_lock ); \
			yield( jobBlocked ); \
		} else \
			unlock_SPINLOCK( &self->waitqueue_lock ); \
	} while(0)

// Inter-job-communication ////////////////////////////////////////////////////

// Internal; do not call directly
#define performch( action, chan, size, p )	  \
	do { \
		set_duff( &self->fibre ); \
		int ret = (action)( self, (chan), (size), (p) ); \
		if( channelBlocked == ret ) \
			yield( jobBlocked ); \
	} while(0)

// Read sizeof(`dest`) bytes from `chan` into &`dest`. Blocks until at least
// sizeof(`dest`) bytes are available in the channel's ringbuf.
//
// @chan - job_channel_p to read data from
// @dest - any C datum that is addressable (e.g. can use & on)
#define readch( chan, dest )	  \
	performch( read_Channel, (chan), sizeof( (dest) ), &(dest) )

#define tryreadch( chan, dest )	  \
	(channelBlocked != try_read_Channel( (chan), sizeof( (dest) ), &(dest) ))

#define trywritech( chan, src ) \
	(channelBlocked != try_write_Channel( (chan), sizeof( (src) ), &(src) ))

// Write sizeof(`data`) bytes from &`data` into `chan`. Blocks until at least
// sizeof(`data`) bytes are free in the channel's ringbuf.
//
// @chan - job_channel_p to write data to
// @data - any C datum that is addressable (e.g. can use & on)
#define writech( chan, data ) \
	performch( write_Channel, (chan), sizeof( (data) ), &(data) )

// Read `size` bytes from `chan` into `dest`. Blocks until at least
// `size` bytes are available in the channel ringbuf.
//
// @chan - job_channel_p to read data from
// @size - number of bytes to read into &`dest`
// @dest - pointer to destination buffer
#define readch_raw( chan, size, dest ) \
	performch( read_Channel, (chan), (size), (dest) )

// Write `size` bytes from `data` into `chan`. Blocks until at least
// `size` bytes are free in the channel ringbuf.
//
// @chan - job_channel_p to write data to
// @size - number of bytes to read into &`dest`
// @data - pointer to data to be written
#define writech_raw( chan, size, data ) \
	performch( write_Channel, (chan), (size), (data) )

// Block until there is some activity on one of the channels in `chanalt`
//
// @chanalt - pointer to Chanalt* to wait on
#define altch( chanalt ) \
	do { \
		set_duff( &self->fibre ); \
		int ret = alt_Channel( self, (chanalt) ); \
		if( channelBlocked == ret ) \
			yield( jobBlocked ); \
	} while(0)

#endif
