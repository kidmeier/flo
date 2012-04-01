#include <stdlib.h>
#include <sys/types.h>

#include "core.features.h"
#include "core.log.h"
#include "sync.once.h"
#include "sync.spinlock.h"

#if defined( feature_WIN32 )
#include <windows.h>
#endif	

#if defined(feature_TRACE)
static logLevel_e    level = logTrace;
static bool abort_on_fatal = true;
#elif defined(feature_DEBUG)
static logLevel_e    level = logDebug;
static bool abort_on_fatal = true;
#else
static logLevel_e    level = logWarning;
static bool abort_on_fatal = false;
#endif

#if defined( feature_WIN32 )
static HANDLE          log_fp = INVALID_HANDLE_VALUE;
#else
static FILE*           log_fp = NULL;
#endif

// Internal bits

static void _do_init( void ) {

#if defined( feature_WIN32 )
	AllocConsole();
	log_fp = GetStdHandle( STD_ERROR_HANDLE );
#endif	

	log_fp = stderr;	

}	

static void init_log( void ) {

	once( _do_init );

}

// Public API /////////////////////////////////////////////////////////////////

void  set_LOG_fatal_abort( bool _abort_on_fatal ) {
	abort_on_fatal = _abort_on_fatal;
}

void  set_LOG_output_fp( FILE* fp ) {

	init_log();
	if( NULL != fp )
		log_fp = fp;

}

void  set_LOG_output( const char* file ) {

	init_log();

	FILE* fp = fopen( file, "w" );
	if( NULL != fp )
		log_fp = fp;

}

void  set_LOG_level( logLevel_e _level ) {

	level = _level;

}

void write_LOG( logLevel_e severity, const char* fmt, const char* file, int lineno, va_list vargs ) {

	static const char* level_map[] = {
		[logFatal]   = "FATAL",
		[logError]   = "ERROR",
		[logWarning] = "WARNING",
		[logInfo]    = "INFO",
		[logDebug]   = "DEBUG",
		[logTrace]   = "TRACE"
	};

	if( severity > level )
		return;

	// Make sure log_fp and filter is initialized
	init_log();

	// It all checks out
	char msg[4096];
	vsprintf( msg, fmt, vargs );

#if defined( feature_WIN32 )
	char line[4096];
	sprintf( line, "[%s %s:%d] %s\n", level_map[severity], file, lineno, msg );
	
	DWORD written; WriteConsole( log_fp, line, strlen(line), &written, NULL );
#else

	fprintf( log_fp, "[%s %s:%d] %s\n", level_map[severity], file, lineno, msg );

#endif

	if( logFatal == severity 
		&& abort_on_fatal ) {
		abort();
	}

}
