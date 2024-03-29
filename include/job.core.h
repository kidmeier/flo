#ifndef __job_core_H__
#define __job_core_H__

#include "core.types.h"
#include "data.list.h"
#include "data.handle.h"
#include "job.fibre.h"
#include "mm.region.h"
#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.spinlock.h"

typedef enum {

	jobBlocked = -1, // Job is waiting for some event or condition
	jobNew,
	jobWaiting,      // Job is runnable but not currently executing
	jobRunning,      // Job is executing at this moment
	jobYielded,      // Job has yielded itself to allow others to execute
	jobCancelled,    // Job has been asynchronously cancelled
	jobExited,       // Job has exited ( called exit_job(..) )
	jobDone,         // Job function ran to completion

} jobstatus_e;

typedef enum {

	cpuBound,  // Hint to the scheduler that this job will be cpu bound
	ioBound,   // Hint to the scheduler that this job will be io bound

	stickyJob, // Request to be always be run on the same thread

	maxJobClass

} jobclass_e;

// The Job type should be treated by clients as an opaque structure.
// It's full definition is included here by the necessity, as the macro job 
// DSL could not compile without its full definition.
// ////////////////////////////////////////////////////////////////////////////
typedef struct Job Job;

typedef int   (*jobfunc_f)( Job*, void*, void*, void** );

struct Job {

	uint32      id;

	// Job control
	fibre_t     fibre;

	spinlock_t  lock;
	jobstatus_e status;

	spinlock_t  waitqueue_lock;
	List*       waitqueue;      // List of other Jobs waiting on this job

	uint32      deadline;
	jobclass_e  jobclass;

	// Job data
	region_p    R;
	pointer     result_p;
	jobfunc_f   run;
	pointer     params;
	pointer     locals;

	bool        cancelled;

	char        pad[10];
};

// API ////////////////////////////////////////////////////////////////////////

int             init_Jobs( int n_workers );
void        shutdown_Jobs(void);

Handle          call_Job( Job*, uint32, jobclass_e, void*, jobfunc_f, void* );
Handle        submit_Job( uint32, jobclass_e, void*, jobfunc_f, void* );

// Set the job's cancelled flag. The job can query for this condition via
// the `is_cancelled' macro and take appropriate action. In either case the
// job is cancelled after it relinquishes its current timeslice.
//
// This is an asynchronous operation and no guarantees are made wrt to the
// order that a job reacts to or is notified of this occurrence.
//
// @jid - Handle of the job to cancel
//
void          cancel_Job( Handle job );

// Blocks until all jobs with the specified deadline have completed. Caller
// provides mutex,condition pair for synchronization:
//
// - mutex must already be locked before calling this function
// - signal is the condition that will be signalled when deadline has completed
int    join_deadline_Job( uint32 deadline, mutex_t* mutex, condition_t* signal );

#endif
