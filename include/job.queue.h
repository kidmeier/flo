#ifndef __job_queue_h__
#define __job_queue_h__

#include "data.list.h"
#include "job.core.h"
#include "job.fibre.h"
#include "sync.spinlock.h"
#include "time.core.h"

// Job queue //////////////////////////////////////////////////////////////////
//
// The job_queue_s type should be treated by clients as an opaque structure.
// It's full definition is included here by the necessity, as the macro job 
// DSL could not compile without its full definition.
// ////////////////////////////////////////////////////////////////////////////

struct job_queue_s {

	uint32           id;
	fibre_t          fibre;

	uint32           deadline;
	jobclass_e       jobclass;

	void*            result_p;
	jobfunc_f        run;
	void*            params;
	void*            locals;

	struct job_queue_s*  parent;

	jobstatus_e status;

	spinlock_t           waitqueue_lock;
	struct job_queue_s*  waitqueue;

	llist_mixin( struct job_queue_s );

};

int   init_JOB_queue(void);
void  insert_JOB( job_queue_p job );
job_queue_p dequeue_JOB( usec_t timeout );
jobid alloc_JOB( job_queue_p, uint32, jobclass_e, void*, jobfunc_f, void* );
void  free_JOB( job_queue_p job );

void sleep_waitqueue_JOB( spinlock_t* wq_lock, job_queue_p* waitqueue, job_queue_p job );
void wakeup_waitqueue_JOB( spinlock_t* wq_lock, job_queue_p* waitqueue );

#endif
