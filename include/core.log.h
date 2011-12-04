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
printLog( logLevel_e level, const char* fmt, const char* file, int lineno, ... ) {

	va_list vargs;

	va_start( vargs, lineno );
	write_LOG( level, fmt, file, lineno, vargs );
	va_end( vargs );

}

// Error reporting
#define fatal( fmt, ... )	  \
	printLog( logFatal, fmt, __FILE__, __LINE__, __VA_ARGS__ )

#define fatal0( str )	  \
	printLog( logFatal, str, __FILE__, __LINE__ )

#define error( fmt, ... )	  \
	printLog( logError, fmt, __FILE__, __LINE__, __VA_ARGS__ )

#define error0( str )	  \
	printLog( logError, str, __FILE__, __LINE__ )

#define warning( fmt, ... )	  \
	printLog( logWarning, fmt, __FILE__, __LINE__, __VA_ARGS__ )

#define warning0( str )	  \
	printLog( logWarning, str, __FILE__, __LINE__ )

#define info( fmt, ... )	  \
	printLog( logInfo, fmt, __FILE__, __LINE__, __VA_ARGS__ )

#define info0( str )	  \
	printLog( logInfo, str, __FILE__, __LINE__ )

// Trace and debug statements are only enabled w/ respective #defines

#if defined( feature_DEBUG ) || defined( feature_TRACE )

#define debug( fmt, ... ) \
	printLog( logDebug, fmt, __FILE__, __LINE__, __VA_ARGS__ )

#define debug0( str )	  \
	printLog( logDebug, str, __FILE__, __LINE__ )

#else

#define debug( fmt, ... )

#define debug0( str )

#endif

#ifdef feature_TRACE

#define trace( fmt, ... ) \
	printLog( logTrace, fmt, __FILE__, __LINE__, __VA_ARGS__ )

#define trace0( str )	  \
	printLog( logTrace, str, __FILE__, __LINE__ )

#else // TRACE feature not enabled; 

#define trace( fmt, ... ) 

#define trace0( str )

#endif

#endif
