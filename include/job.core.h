#ifndef __job_core_H__
#define __job_core_H__

#include "sync.condition.h"
#include "sync.mutex.h"

typedef enum {

	jobBlocked = -1,
	jobWaiting,
	jobRunning,
	jobExited,
	jobDone,

} jobstatus_e;

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

// API ////////////////////////////////////////////////////////////////////////

int         init_JOBS( void );
void        shutdown_JOBS(void);

extern jobid nullJob;

jobid       queue_JOB( jobid, uint32, jobclass_e, void*, jobfunc_f, void* );
jobstatus_e status_JOB( jobid );

// Blocks until all jobs with the specified deadline have completed. Caller
// provides mutex,condition pair for synchronization:
//
// - mutex must already be locked before calling this function
// - signal is the condition that will be signalled when deadline has completed
int   join_deadline_JOB( uint32 deadline, mutex_t* mutex, condition_t* signal );

#endif
