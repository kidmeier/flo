#ifndef __parse_core_h__
#define __parse_core_h__

#include "core.types.h"

enum parse_status_e {

	parseOk,
	parseFailed,
	parsePartial,

};

typedef struct parse_error_s  parse_error_t;
typedef struct parse_error_s* parse_error_p;
struct parse_error_s {

	const char* msg;
	const char* line;

	int lineno;
	int col;

	struct parse_error_s* next;

};

typedef struct parse_s  parse_t;
typedef struct parse_s* parse_p;
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

// Predicates
typedef int (*charpred_f)( int ch );

parse_p new_string_PARSE( const char* s );
parse_p new_buf_PARSE( int sz, const char* buf );

void    destroy_PARSE( parse_p p );

parse_p integer( parse_p P, int* i );
parse_p decimalf( parse_p P, float* f );
parse_p decimald( parse_p P, double* d );
parse_p skipws( parse_p P );
parse_p string( parse_p P, charpred_f delimiterf, char** s );
parse_p qstring( parse_p P, char** s );
parse_p match( parse_p P, const char* s );
parse_p matchc( parse_p P, const char c );
parse_p matchone( parse_p P, const char* chars, char* c );

bool    trymatchc( parse_p P, const char c );

parse_p parselect( parse_p P, int optc, const char *optv[], int *choice );

char    peek( parse_p P );
char    lookahead( parse_p P, int diff );

static inline
bool          parsok( const parse_p P ) { return parseOk == P->status; }
bool          parseof( const parse_p P );
parse_p       parsync( parse_p P, const char sync, parse_error_p err );
parse_error_p parserr( parse_p P, const char* msg, ... );
int           parserrc( const parse_p P );
parse_error_p parserri( const parse_p P, int ierr );

#endif
