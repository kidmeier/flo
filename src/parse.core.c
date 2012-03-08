#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "control.maybe.h"
#include "core.types.h"
#include "parse.core.h"

parse_p new_string_PARSE( const char* s ) {
	
	return new_buf_PARSE( strlen(s), s );

}

parse_p new_buf_PARSE( int sz, const char* buf ) {

	parse_p P = malloc( sizeof(parse_t) );

	memset( P, 0, sizeof(parse_t) );
	
	P->begin = buf;
	P->pos = buf;
	P->eof = buf + sz;

	P->line = buf;
	P->lineno = 0;
	P->col = 0;

	P->status = parseOk;

	return P;
}

void    destroy_PARSE( parse_p P ) {

	struct parse_error_s *err = P->error; 
	while( NULL != err ) {

		struct parse_error_s *next = err->next;

		free( err );
		err = next;

	}

	free( P );

}

static inline
bool eofp( const char* p, const char* eof ) {
	return p >= eof;
}

static inline
bool eof( parse_p P ) {

	return eofp( P->pos, P->eof );

}

static inline 
parse_p advance( parse_p P ) {

	if( eof(P) )
		return P;

	switch( *P->pos ) {

	case '\n':
		P->lineno ++;
		P->col = 0;
		P->line = P->pos + 1;
		break;

	default:
		P->col ++;
		break;

	}
	
	P->pos ++;
	return P;

}

static inline 
parse_p advance_to( parse_p P, const char* to ) {

	while( !eof(P) && P->pos < to ) 
		advance(P);

	return P;

}

static inline
parse_p retreat( parse_p P ) {

	// Nothing more if we are at the beginning
	if( P->pos <= P->begin ) {

		P->lineno = 0;
		P->col = 0;
		P->line = P->begin;

		return P;

	}

	P->pos --;

	switch( *P->pos ) {
	case '\n': {
		const char* line = P->pos - 1;
		while( *line != '\n' && line > P->begin ) 
			line--;
		P->lineno--;
		P->col = P->pos - line;
		P->line = line;

		break;
	}
	default:
		P->col--;
		break;
	}
	
	return P;

}

static inline
parse_p retreat_to( parse_p P, const char* to ) {

	while( P->pos >= P->begin && P->pos > to )
		retreat(P);

	return P;

}

static inline
parse_p fforward( parse_p P, int N ) {

	for( int i=0; i<N; i++ )
		advance(P);
	
	return P;

}

static inline
parse_p rrewind( parse_p P, int N ) {

	for( int i=0; i<N; i++ )
		retreat(P);

	return P;

}

parse_p integer( parse_p P, int* i ) {

	if( !parsok(P) ) 
		return P;

	if( eof(P) ) {

		P->status = parsePartial;
		return P;

	}

	char* endp; long l = strtol( P->pos, &endp, 10 );
	if( endp == P->pos ) {
		
		P->status = parseFailed;
		return P;

	}

	if( i )
		*i = (int)l;
	
	return advance_to( P, endp );

}

parse_p uinteger( parse_p P, unsigned int *ui ) {

	int i; P = integer( P, &i );

	if( !parsok(P) )
		return P;

	if( i < 0 ) {
		P->status = parseFailed;
		return P;
	}

	if( ui )
		*ui = (unsigned int)i;

	return P;

}

parse_p decimalf( parse_p P, float* f ) {

	if( !parsok(P) ) 
		return P;

	if( eof(P) ) {

		P->status = parsePartial;
		return P;

	}

	char* endp;
	double d = strtod( P->pos, &endp );
	
	if( endp == P->pos ) {

		P->status = parseFailed;
		return P;

	}

	if( f )
		*f = (float)d;

	return advance_to( P, endp );

}

parse_p decimald( parse_p P, double* d ) {

	if( !parsok(P) )
		return P;

	if( eof(P) ) {

		P->status = parsePartial;
		return P;

	}

	char* endp;
	double dd = strtod( P->pos, &endp );
	
	if( endp == P->pos ) {
		
		P->status = parseFailed;
		return P;

	}

	if( d )
		*d = dd;
	
	return advance_to( P, endp );

}

parse_p skipws( parse_p P ) {

	if( !parsok(P) )
		return P;

	while( !eof(P) && isspace(*P->pos) )
		advance(P);

	return P;

}

parse_p string( parse_p P, charpred_f delimiterf, char** s ) {

	if( !parsok(P) )
		goto failed;

	if( eof(P) ) {

		P->status = parsePartial;
		goto failed;

	}

	if( delimiterf(*P->pos) ) {

		P->status = parseFailed;
		goto failed;

	}

	const char* begin = P->pos;
	do {

		advance(P);
		if( eof(P) ) {

			P->status = parsePartial;
			goto failed;

		}

	} while( !delimiterf(*P->pos) );

	if( s ) {

		int length = P->pos - begin;
		*s = malloc( length+1 );
		strncpy( *s, begin, length );
		(*s)[length] = '\0';

	}
	return P;

failed:
	if( s ) 
		(*s) = NULL;
	return P;

}

static inline
int isquot( int ch ) {
	return '"' == ch;
}

parse_p qstring( parse_p P, char** s ) {

	if( !isquot(*P->pos) ) {
		P->status = parseFailed;
		return P;
	}
	advance(P);

	P = string( P, isquot, s );
	if( parsok(P) )
		advance(P);

	return P;

}

parse_p match( parse_p P, const char* s ) {

	if( !parsok(P) )
		return P;

	while( '\0' != *s ) {

		if( eof(P) ) {

			P->status = parsePartial;
			return P;

		}

		if( *P->pos != *s ) {

			P->status = parseFailed;
			return P;

		}

		s ++; advance(P);

	}

	return P;

}

parse_p matchc( parse_p P, const char c ) {

	if( !parsok(P) )
		return P;

	if( eof(P) ) {
		
		P->status = parsePartial;
		return P;

	}

	if( c != *P->pos ) {

		P->status = parseFailed;
		return P;

	}
	advance(P);

	return P;

}

parse_p matchone( parse_p P, const char* chars, char* c ) {

	const char* ch = chars;
	while( '\0' != *ch && !parsok( matchc( P, *ch) ) )
		ch ++;
	
	if( c )
		*c = *ch;

	return P;

}

bool    trymatchc( parse_p P, const char c ) {

	enum parse_status_e status = P->status;

	if( parsok( matchc(P, c) ) )
		return true;

	// Restore prior status
	P->status = status;
	return false;

}

parse_p parselect( parse_p P, int optc, const char *optv[], int *choice ) {

	for( int i=0; i<optc; i++ ) {

		const char *opt = optv[i];

		// Save state so we can restore if this option fails
		parse_t Pstate = (*P);

		if( parsok( match(P, opt) ) ) {

			if( choice )
				(*choice) = i;
			return P;

		}

		// Restore and try again
		(*P) = Pstate;

	}

	// No match
	P->status = parseFailed;
	return P;

}

char    lookahead( parse_p P, int diff ) {

	if( eofp(P->pos+diff, P->eof) )
		return 0;

	return *(P->pos + diff);

}

char    peek( parse_p P ) {

	return lookahead(P, 0);

}

bool          parseof( const parse_p P ) {

	return eof(P);

}

parse_p       parsync( parse_p P, const char sync, parse_error_p err ) {

	if( eof(P) ) 
		return P;

	do {

		advance(P);

	} while( !eof(P) && sync != *P->pos );

	if( err ) {

		err->next = P->error;
		P->error  = err;
		P->errcount ++;

	}

	P->status = parseOk;
	return P;

}

parse_error_p parserr( parse_p P, const char* fmt, ... ) {

	char message     [2048];
	char error_string[2048];

	va_list args;

	va_start(args, fmt);
	int ret = vsnprintf( message, sizeof(message), fmt, args );
	va_end(args);
	
	ret = maybe( ret, < 0,
	             snprintf( error_string, sizeof(error_string), "%s", message )
		);

	// Find the end of the current line
	const char* endp = P->pos;
	while( endp < P->eof && '\n' != *endp )
		endp ++;

	// Allocate the error struct and all required strings as one contiguous block of memory
	pointer errbuf = malloc( sizeof(parse_error_t) 
	                         + strlen(error_string)+1 
	                         + (endp - P->line + 1) );
	parse_error_p err = errbuf;
	char *msg  = errbuf + sizeof(parse_error_t);
	char *line = errbuf + sizeof(parse_error_t) + strlen(error_string)+1;

	strcpy ( msg, error_string );
	strncpy( line, P->line, endp - P->line );
	line[ endp - P->line ] = '\0';

	err->msg    = msg;
	err->line   = line;
	err->lineno = P->lineno;
	err->col    = P->col;

	err->next = P->error;
	P->error = err;
	P->errcount++;

	return err;

}

int           parserrc( parse_p P ) {

	return P->errcount;

}

parse_error_p parserri( parse_p P, int ierr ) {

	parse_error_p err = P->error;
	for( int i=0; i<P->errcount - ierr - 1; i++ ) {

		if( !err )
			return NULL;

		err = err->next;

	}
	return err;

}

#ifdef __parse_core_TEST__

int main( int argc, char* argv[] ) {

	const char* s = " [section] \n  \"var\"=12 \n2.5 + \"foo\"";
	parse_p P = new_string_PARSE(s);

	printf("begin         % 2d,% 2d|%s\n", P->lineno, P->col, P->pos);
	printf("skipws        % 2d,% 2d|%s\n", P->lineno, P->col, skipws(P)->pos);
	printf("skipws        % 2d,% 2d|%s\n", P->lineno, P->col, skipws(P)->pos);
	printf("matchc [      % 2d,% 2d|%s\n", P->lineno, P->col, matchc(P, '[')->pos);
	printf("match section % 2d,% 2d|%s\n", P->lineno, P->col, match(P, "section")->pos);
	printf("matchc ]      % 2d,% 2d|%s\n", P->lineno, P->col, matchc(P, ']')->pos);
	printf("skipws        % 2d,% 2d|%s\n", P->lineno, P->col, skipws(P)->pos);
	printf("qstring       % 2d,% 2d|%s\n", P->lineno, P->col, qstring(P, NULL, NULL)->pos);
	printf("matchc =      % 2d,% 2d|%s\n", P->lineno, P->col, matchc(P, '=')->pos);
	printf("integer       % 2d,% 2d|%s\n", P->lineno, P->col, integer(P, NULL)->pos);
	printf("skipws        % 2d,% 2d|%s\n", P->lineno, P->col, skipws(P)->pos);
	printf("decimalf      % 2d,% 2d|%s\n", P->lineno, P->col, decimalf(P, NULL)->pos);
	printf("skipws        % 2d,% 2d|%s\n", P->lineno, P->col, skipws(P)->pos);
	printf("decimalf      % 2d,% 2d|%s\n", P->lineno, P->col, decimalf(P, NULL)->pos);
	printf("qstring       % 2d,% 2d|%s\n", P->lineno, P->col, qstring(P, NULL, NULL)->pos);
	printf("parsync +     % 2d,% 2d|%s\n", P->lineno, P->col, parsync(P, '+', NULL)->pos);
	printf("matchc +      % 2d,% 2d|%s\n", P->lineno, P->col, matchc(P, '+')->pos);
	printf("qstring       % 2d,% 2d|%s\n", P->lineno, P->col, qstring(P, NULL, NULL)->pos);
	printf("eof           % 2d,% 2d|%d\n", P->lineno, P->col, P->pos >= P->eof );

	destroy_PARSE(P);

}

#endif
