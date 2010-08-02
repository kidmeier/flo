#ifndef __job_fibre_h__
#define __job_fibre_h__

// Duff's device; the backbone of fibres //////////////////////////////////////

typedef unsigned short duff_t;

#define init_duff( duff ) \
	*(duff) = 0

#define resume_duff( duff ) \
	switch( *(duff) ) { \
	case 0:

#define set_duff( duff ) \
	*(duff) = __LINE__; \
	case __LINE__:

#define end_duff( duff ) \
	}

// Fibres - stackless threads built on Duff's device //////////////////////////

typedef duff_t fibre_t;

// Administrivia //////////////////////////////////////////////////////////////

#define init_fibre( fibre ) \
	init_duff( fibre )

#define begin_fibre( fibre )	  \
	{ \
	resume_duff( fibre )

#define end_fibre( fibre ) \
	end_duff( fibre ); \
	init_fibre( fibre ); \
	return jobDone; \
	}

// Fibre control flow /////////////////////////////////////////////////////////

#define restart_fibre( fibre ) \
	do { \
		init_fibre( fibre ); \
		return jobYielded; \
	} while(0) 

#define exit_fibre( fibre ) \
	do { \
		init_fibre( fibre ); \
		return jobExited; \
	} while(0)

#define yield_fibre( fibre, status )	  \
		return (status)

// Fibre choreography /////////////////////////////////////////////////////////

#define busywait_until( fibre, cond, status )	  \
	do { \
		set_duff( fibre ); \
		if( !(cond) ) { \
			yield_fibre( (fibre), (status) ); \
		} \
	} while( 0 )

#define busywait_while( fibre, cond, status )	  \
	busywait_until( (fibre), !(cond), (status) )

#endif
