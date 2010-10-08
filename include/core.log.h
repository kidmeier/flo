#ifndef __core_log_h__
#define __core_log_h__

#include <stdarg.h>
#include <stdio.h>
//
#include "core.features.h"
#include "core.types.h"

typedef enum {

	logFatal,
	logError,
	logWarning,
	logInfo,
	logDebug,
	logTrace

} logLevel_e;

void  set_LOG_fatal_abort( bool abort_on_fatal );
void  set_LOG_output_fp( FILE* fp );
void  set_LOG_output( const char* file );
void  set_LOG_level( logLevel_e level );
int   set_LOG_filter( const char* filter );
void write_LOG( logLevel_e  level, 
                const char* fmt, 
                const char* file, 
                int         lineno, 
                va_list     vargs );

// Macro-level API ////////////////////////////////////////////////////////////

static void inline
_log( logLevel_e level, const char* fmt, const char* file, int lineno, ... ) {

	va_list vargs;

	va_start( vargs, lineno );
	write_LOG( level, fmt, file, lineno, vargs );
	va_end( vargs );

}

// Error reporting
#define fatal( fmt, args... )	  \
	_log( logFatal, fmt, __FILE__, __LINE__, args )

#define fatal0( str )	  \
	_log( logFatal, str, __FILE__, __LINE__ )

#define error( fmt, args... )	  \
	_log( logError, fmt, __FILE__, __LINE__, args )

#define error0( str )	  \
	_log( logError, str, __FILE__, __LINE__ )

#define warning( fmt, args... )	  \
	_log( logWarning, fmt, __FILE__, __LINE__, args )

#define warning0( str )	  \
	_log( logWarning, str, __FILE__, __LINE__ )

#define info( fmt, args... )	  \
	_log( logInfo, fmt, __FILE__, __LINE__, args )

#define info0( str )	  \
	_log( logInfo, str, __FILE__, __LINE__ )

// Trace and debug statements are only enabled w/ respective #defines

#ifdef feature_DEBUG

#define debug( fmt, args... ) \
	_log( logDebug, fmt, __FILE__, __LINE__, args )

#define debug0( str )	  \
	_log( logDebug, str, __FILE__, __LINE__ )

#else

#define debug( fmt, args... )

#define debug0( str )

#endif

#ifdef feature_TRACE

#define trace( fmt, args... ) \
	_log( logTrace, fmt, __FILE__, __LINE__, args )

#define trace0( str )	  \
	_log( logTrace, str, __FILE__, __LINE__ )

#else // TRACE feature not enabled; 

#define trace( fmt, args... ) 

#define trace0( str )

#endif

#endif
