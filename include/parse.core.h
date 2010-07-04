#ifndef __parse_core_h__
#define __parse_core_h__

#include "core.types.h"

enum parse_status_e {

	parseOk,
	parseFailed,
	parsePartial,

};

struct parse_error_s;
typedef struct parse_error_s parse_error_t;
typedef parse_error_t* parse_error_p;
struct parse_error_s {

	const char* msg;
	const char* line;

	int lineno;
	int col;

	struct parse_error_s* next;

};

struct parse_s;
typedef struct parse_s parse_t;
typedef parse_t* parse_p;
struct parse_s {

	const char* begin;
	const char* eof;
	const char* pos;

	const char* line;

	int lineno;
	int col;
	
	enum parse_status_e status;
	parse_error_p       error;
	int                 errcount;
	
};

parse_p new_string_PARSE( const char* s );
parse_p new_buf_PARSE( int sz, const char* buf );

parse_p integer( parse_p P, int* i );
parse_p decimalf( parse_p P, float* f );
parse_p decimald( parse_p P, double* d );
parse_p skipws( parse_p P );
parse_p qstring( parse_p P, const void* pool, char** s );
parse_p match( parse_p P, const char* s );
parse_p matchc( parse_p P, const char c );
parse_p matchone( parse_p P, const char* chars, char* c );

static inline
bool          parsok( const parse_p P ) { return parseOk == P->status; }
parse_p       parsync( parse_p P, const char sync, parse_error_p err );
parse_error_p parserr( parse_p P, const char* msg, ... );
int           parserrc( const parse_p P );
parse_error_p parserri( const parse_p P, int ierr );

#endif
