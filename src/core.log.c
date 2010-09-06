#include <regex.h>
#include <stdlib.h>
#include <sys/types.h>

#include "core.log.h"
#include "sync.atomic.h"

#ifdef DEBUG
static enum loglevel_e level = logTrace;
static bool            abort_on_fatal = true;
#else
static enum loglevel_e level = logWarning;
static bool            abort_on_fatal = false;
#endif

static FILE*           log_fp = NULL;
static regex_t         filter;

// Internal bits

static void init_log( void ) {

	if( atomic_cas(log_fp, NULL, stderr) ) {

		if( 0 != regcomp(&filter, ".*", REG_NOSUB) ) {
			fprintf(stderr, "%s.%d: Unable to initialize default LOG filter\n",
			        __FILE__, __LINE__);
			abort();
		}
		
	}

}

// Public API /////////////////////////////////////////////////////////////////

void set_LOG_fatal_abort( bool _abort_on_fatal ) {
	abort_on_fatal = _abort_on_fatal;
}

void set_LOG_output_fp( FILE* fp ) {

	init_log();
	if( NULL != fp )
		log_fp = fp;

}

void set_LOG_output( const char* file ) {

	init_log();

	FILE* fp = fopen( file, "w" );
	if( NULL != fp )
		log_fp = fp;

}

void set_LOG_level( enum loglevel_e _level ) {

	init_log();
	level = _level;

}

int set_LOG_filter( const char* _filter ) {
	
	init_log();
	return regcomp( &filter, _filter, REG_NOSUB );

}

void write_LOG( enum loglevel_e severity, const char* fmt, const char* file, int lineno, va_list vargs ) {

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

	// Filtered out
	if( 0 != regexec(&filter, file, 0, NULL, 0) )
		return;
	
	// It all checks out
	char msg[4096];
	vsprintf( msg, fmt, vargs );
	fprintf( log_fp, "[%s %s:%d] %s\n", level_map[severity], file, lineno, msg );

	if( logFatal == severity 
		&& abort_on_fatal ) {
		abort();
	}

}
