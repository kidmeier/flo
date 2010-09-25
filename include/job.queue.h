#ifndef __job_queue_h__
#define __job_queue_h__

#include "data.list.h"
#include "job.core.h"
#include "job.fibre.h"
#include "sync.spinlock.h"
#include "time.core.h"

typedef Job* Waitqueue;

// Job queue //////////////////////////////////////////////////////////////////

int               init_Job_queue(void);
void            insert_Job( Job* job );
Job*           dequeue_Job( usec_t timeout );
jobid            alloc_Job( uint32, jobclass_e, void*, jobfunc_f, void* );
void              free_Job( Job* job );

void   sleep_waitqueue_Job( spinlock_t* wq_lock, Waitqueue* waitqueue, Job* job );
void  wakeup_waitqueue_Job( spinlock_t* wq_lock, Waitqueue* waitqueue );

#endif
