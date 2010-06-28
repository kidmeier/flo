#ifndef __parse_core_h__
#define __parse_core_h__

#include "seq.core.h"

enum parse_result_e {

  FULL_PARSE = 0,
  PARTIAL_PARSE = -1,
  FAIL_PARSE = -2

};

struct parse_s {

  enum parse_result_e status;

  union {
    bool        b;
    char        c;
    short       s;
    int         i;
    long        l;
    long long   ll;
    float       f;
    double      d;
    long double ld;
    const char* string;
    any         p;
  } result;

  seq_t rest;
};
typedef struct parse_s parse_t;
typedef parse_t* parse_p;

typedef parse_t (*parser_f)( seq_t, any args );
typedef int     (*parse_action_f)( parse_t result );

struct parser_s {

  parser_f parse;
  any      args;

  parse_action_f action;

};
typedef struct parser_s parser_t;
typedef parser_t* parser_p;

parser_p char_PARSE( any ctx, parse_action_f action, char c );
parser_p charset_PARSE( any ctx, parse_action_f action, const char* charset );
parser_p string_PARSE( any ctx, parse_action_f action, const char* s );
parser_p long_PARSE( any ctx, parse_action_f action );
parser_p longlong_PARSE( any ctx, parse_action_f action );
parser_p float_PARSE( any ctx, parse_action_f action );
parser_p double_PARSE( any ctx, parse_action_f action );
parser_p longdouble_PARSE( any ctx, parse_action_f action );

parser_p maybe_PARSE( any ctx, parse_action_f action, parser_p parser );
parser_p many0_PARSE( any ctx, parse_action_f action, parser_p parser );
parser_p many1_PARSE( any ctx, parse_action_f action, parser_p parser );
//parser_p manynm_PARSE( any ctx, parse_action_f action, int min, int max, parser_p parser);
parser_p and_PARSE( any ctx, parse_action_f action, int n, ... );
parser_p or_PARSE( any ctx, parse_action_f action, int n, ... );

#endif
