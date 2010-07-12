#ifndef __job_core_H__
#define __job_core_H__

#include <protothread/pt.h>

#include "sync.condition.h"
#include "sync.mutex.h"

enum jobclass_e {
	cpuBound,
	ioBound,

	maxJobClass
};

enum jobstatus_e {
	
	jobWaiting,
	jobRunning,
	jobDone,

};

typedef struct job_queue_s job_queue_t;
typedef job_queue_t* job_queue_p;

typedef struct {

	uint32       id;
	job_queue_p  job;

} jobid;

typedef struct pt job_control_t;
typedef char (*jobfunc_f)( job_control_t*, void* );

// Job control ////////////////////////////////////////////////////////////////
#define defjob( name, arg ) \
	char name ( job_control_t* self, arg ) 

#define begin_job	\
	PT_BEGIN( self )

#define end_job \
	PT_END( self )

#define wait_until( cond )	  \
	PT_WAIT_UNTIL( self, (cond) )

#define wait_while( cond ) \
	PT_WAIT_WHILE( self, (cond) )

#define wait_job( jid ) \
	PT_WAIT_THREAD( self, (jid).job->run( (jid).job->arg ) )

#define spawn_job( jid, deadline, jobclass, jobfunc, arg )	  \
	do { \
		(jid) = queue_JOB( (deadline), (jobclass), (jobfunc), (arg) ); \
		PT_WAIT_UNTIL( self, jobDone == status_JOB( (jid) ) ); \
	} while( 0 )

/*
#define spawn_job( jobfunc, arg ) \
	PT_WAIT_THREAD( self, (jobfunc)( self, (arg) ) )
*/

#define restart_job \
	PT_RESTART( self )

#define exit_job \
	PT_EXIT( self )

#define yield \
	PT_YIELD( self )

#define yield_until( cond ) \
	PT_YIELD_UNTIL( self, (cond) )

// API ////////////////////////////////////////////////////////////////////////

int              init_JOBS(void);
void             shutdown_JOBS(void);

jobid            queue_JOB( uint32 deadline, enum jobclass_e, jobfunc_f, void* );
enum jobstatus_e status_JOB( jobid );

// Blocks until all jobs with the specified deadline have completed. Caller
// provides mutex,condition pair for synchronization:
//
// - mutex must already be locked before calling this function
// - signal is the condition that will be signalled when deadline has completed
int   join_deadline_JOB( uint32 deadline, mutex_t* mutex, condition_t* signal );

#endif
