#ifndef __job_core_H__
#define __job_core_H__

#include "job.fibre.h"
#include "sync.condition.h"
#include "sync.mutex.h"

typedef enum {
	cpuBound,
	ioBound,

	maxJobClass
} jobclass_e;

typedef struct job_queue_s job_queue_t;
typedef job_queue_t* job_queue_p;

typedef struct {

	uint32      id;
	job_queue_p job;

} jobid;

typedef char (*jobfunc_f)( job_queue_p, void*, void*, void** );

// Job control ////////////////////////////////////////////////////////////////
//
// The macros below define a DSL job language. There is support for launching
// child jobs, waiting on existing jobs, etc. This is all implemented on top
// of Protothreads, a lightweight co-operative threading construct.
//
// The limitation of this is that there is no stack for local variables. 
// Instead the job control DSL provides for declaring a set of locals and 
// parameters  which are macro-logically bundled into struct's. Access to 
// locals and  parameters must occur through accessor macros.
//
// ////////////////////////////////////////////////////////////////////////////

// Corresponds to function prototype, except it is NOT optional. This is where
// you can declare parameters to the job.
//
// To make your job callable from another compilation unit you can place this in
// a header to be included by consumers of your job.
//
// @name       - C identifier; declares C function that implements the job
// @returntype - C type expression specifying return type
// @params     - Brace-enclosed C-struct body; lists named parameters
#define declare_job( name, returntype, params )	  \
	typedef struct { \
		params \
	} name##_job_params_t; \
	typedef struct name##_job_locals_s name##_job_locals_t; \
	char name ( job_queue_p, returntype *, name##_job_params_t*, name##_job_locals_t** )

// Corresponds to an actual function definition. All calls to this macro must 
// have a preceding call to declare_job in the same compilation unit with the 
// same @name and @returntype.
//
// @name       - C-identifier; defines C function that implements the job
// @returntype - C-type expression specifying the return type
// @locals     - Brace-enclosed C-struct body; lists local vars needed by job
#define define_job( name, returntype, locals )	  \
	struct name##_job_locals_s { \
		locals \
	}; \
	char name ( job_queue_p self, returntype * result, name##_job_params_t* _job_params, name##_job_locals_t** _job_locals ) { \
	if( !(*_job_locals) ) (*_job_locals) = new(NULL, name##_job_locals_t);

// Declares the beginning of a job definition. Must be the first statement 
// in the body of define_job
#define begin_job	  \
	begin_fibre( &self->fibre ) 

// Marks the end of a job definition. Must be the last statement 
// in the body of define_job
#define end_job \
		if( self->parent ) delete( _job_params ); \
		delete( (*_job_locals) ); \
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
		if( self->parent ) delete( _job_params ); \
		delete( (*_job_locals) ); \
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
	busywait_until( &self->fibre, (cond) )

// Yields this job as long as @cond, when coerced to a boolean evaluates 
// to logical true.
//
// @cond - C-expression
#define wait_while( cond ) \
	busywait_while( &self->fibre, (cond) )

// Yields this job until the job referred to by @jid completes.
//
// jid - expression of type jobid
#define wait_job( jid ) \
	busyjoin( &self->fibre, \
	          (jid).job->run( (jid).job, \
	                          &(jid).job->result_p, \
	                          (jid).job->params, \
	                          &(jid).job->locals ) )

// Launch a new child job and yield this job until it completes.
//
// @jid      - jobid; stores the jobid of the child job
// @deadline - uint32; indicate the deadline of this job
// @jobclass - jobclass_e;
// @result_p - pointer to memory to hold job result; can be NULL
// @jobfunc  - name declared via @declare_job in same compilation unit
// @args     - C-struct initializer; initializes job parameters
#define spawn_job( jid, deadline, jobclass, result_p, jobfunc, args )	  \
	do { \
		jobfunc##_job_params_t* params = new(NULL, jobfunc##_job_params_t); \
		*(params) = (jobfunc##_job_params_t) args ; \
		(jid) = queue_JOB( self, (deadline), (jobclass), (result_p), (jobfunc_f)(jobfunc), params ); \
		busywait_until( &self->fibre, jobDone == status_JOB( (jid) ) ); \
	} while(0)

// Yield this job to allow other(s) to run.
#define yield \
	yield_fibre( &self->fibre )

// API ////////////////////////////////////////////////////////////////////////

int         init_JOBS( void );
void        shutdown_JOBS(void);

jobid       queue_JOB( job_queue_p, uint32, jobclass_e, void*, jobfunc_f, void* );
jobstatus_e status_JOB( jobid );

// Blocks until all jobs with the specified deadline have completed. Caller
// provides mutex,condition pair for synchronization:
//
// - mutex must already be locked before calling this function
// - signal is the condition that will be signalled when deadline has completed
int   join_deadline_JOB( uint32 deadline, mutex_t* mutex, condition_t* signal );

#endif
