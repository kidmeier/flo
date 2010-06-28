#include <stdlib.h>

#include "core.alloc.h"
#include "parse.core.h"
#include "seq.array.h"

// Parsers

static parse_t parse_char( seq_t in, any args ) {

  char c = (char)(int)args;
  
  if( *(char*)first(in) == c ) {

    return (parse_t){ .status = FULL_PARSE,
	.result.c = c,
	.rest = rest(in)
	};

  } else {

    return (parse_t){ .status = FAIL_PARSE,
	.result.c = c,
	.rest = in
	};

  }

}

static parse_t parse_charset( seq_t in, any args ) {

  const char* charset = (const char*)args;
  const char* c = strchr(charset, *(char*)first(in));;

  if( NULL != c )
  
    return (parse_t){ .status = FULL_PARSE,
	.result.c = *c,
	.rest = rest(in)
	};
 
  else 

    return (parse_t){ .status = FAIL_PARSE,
	.result.c = 0,
	.rest = in
	};

} 

static parse_t parse_string( seq_t in, any args ) {

  const char* s = (const char*)args;

  for( ; *s && !nil(in); s++, in = rest(in) ) {

    if( *s != *(char*)first(in) )
      return (parse_t){ .status = FAIL_PARSE,
	  .result.string = NULL,
	  .rest = in
	  };

  }

  if( *s ) // partial parse (we hit nil before exhausting s)
    return (parse_t){ .status = PARTIAL_PARSE,
	.result.string = NULL,
	.rest = in
	};
  else
    return (parse_t){ .status = FULL_PARSE,
	.result.string = s,
	.rest = in
	};

}

static int parse_number( seq_t* in, int maxlen, char* number, bool decimal ) {
  
  const char* digits = "0123456789";
  char digit = *(char*)first(*in);
  char* n = &number[0];

  // Negative?
  if( '-' == digit ) {
    *in = rest(*in);
    digit = *(char*)first(*in);

    *n++ = '-';
  }

  while( n - number < maxlen-1 && !nil(*in) && strchr("0123456789", digit) ) {

    *n++ = digit;

    *in = rest(*in);
    digit = *(char*)first(*in);

  }

  // (maybe) Parse decimal
  if( '.' == digit && decimal ) {
    
    *n++ = digit;
    digit = *(char*)first(*in);
    while( n - number < maxlen-1 && !nil(*in) && strchr("0123456789", digit) ) {

      *n++ = digit;
      *in = rest(*in);
      digit = *(char*)first(*in);

    }

  }

  return n - number;

}

static parse_t parse_long( seq_t in, any args ) {

  char number[256];
  int len = parse_number( &in, 256, number, false);

  if( !nil(in) && len < 2 )
    return (parse_t){ .status = FAIL_PARSE,
	.result.l = 0,
	.rest = in
	};

  if( nil(in) ) 
    return (parse_t){ .status = PARTIAL_PARSE,
	.result.l = strtol(number,NULL,0),
	.rest = in
	};

  return (parse_t){ .status = FULL_PARSE,
      .result.l = strtol(number,NULL,0),
      .rest = in
      };

}

static parse_t parse_longlong( seq_t in, any args ) {

  char number[256];
  int len = parse_number( &in, 256, number, false);

  if( !nil(in) && len < 2 )
    return (parse_t){ .status = FAIL_PARSE,
	.result.ll = 0,
	.rest = in
	};

  if( nil(in) ) 
    return (parse_t){ .status = PARTIAL_PARSE,
	.result.ll = strtoll(number,NULL,0),
	.rest = in
	};

  return (parse_t){ .status = FULL_PARSE,
      .result.ll = strtoll(number,NULL,0),
      .rest = in
      };

}

static parse_t parse_float( seq_t in, any args ) {

  char number[256];
  int len = parse_number( &in, 256, number, true );

  if( !nil(in) && len < 2 )
    return (parse_t){ .status = FAIL_PARSE,
	.result.f = 0.0f,
	.rest = in
	};

  if( nil(in) ) 
    return (parse_t){ .status = PARTIAL_PARSE,
	.result.f = strtof(number,NULL),
	.rest = in
	};

  return (parse_t){ .status = FULL_PARSE,
      .result.f = strtof(number,NULL),
      .rest = in
      };

}

static parse_t parse_double( seq_t in, any args ) {

  char number[256];
  int len = parse_number( &in, 256, number, true);

  if( !nil(in) && len < 2 )
    return (parse_t){ .status = FAIL_PARSE,
	.result.d = 0.0,
	.rest = in
	};

  if( nil(in) ) 
    return (parse_t){ .status = PARTIAL_PARSE,
	.result.d = strtod(number,NULL),
	.rest = in
	};

  return (parse_t){ .status = FULL_PARSE,
      .result.d = strtod(number,NULL),
      .rest = in
      };

}

static parse_t parse_longdouble( seq_t in, any args ) {

  char number[256];
  int len = parse_number( &in, 256, number, true);

  if( !nil(in) && len < 2 )
    return (parse_t){ .status = FAIL_PARSE,
	.result.ld = 0,
	.rest = in
	};

  if( nil(in) ) 
    return (parse_t){ .status = PARTIAL_PARSE,
	.result.ld = strtold(number,NULL),
	.rest = in
	};

  return (parse_t){ .status = FULL_PARSE,
      .result.ld = strtold(number,NULL),
      .rest = in
      };

}

static parse_t parse_many0( seq_t in, any args ) {

  parser_p inferior_parser = (parser_p)args;
  parse_t inferior_result;
  int count = -1;

  inferior_result.rest = in;
  do {

    count++;
    inferior_result = inferior_parser->parse( inferior_result.rest, inferior_parser->args );

  } while( inferior_result.status == FULL_PARSE );

  return (parse_t){ inferior_result.status == PARTIAL_PARSE ? PARTIAL_PARSE : FULL_PARSE,
      .result.i = count,
      .rest = inferior_result.rest
      };

}

static parse_t parse_many1( seq_t in, any args ) {

  parse_t parse = parse_many0( in, args );
  if( !parse.result.i ) {
    return (parse_t){ FAIL_PARSE,
	.rest = parse.rest
	};
  }

  return parse;
}

static parse_t parse_maybe( seq_t in, any args ) {

  parser_p inferior_parser = (parser_p)args;
  parse_t  inferior_result = inferior_parser->parse( in, inferior_parser->args );

  if( FAIL_PARSE == inferior_result.status )
    return (parse_t){ .status = FULL_PARSE,
	.result.b = false,
	.rest = in
	};

  return inferior_result;

}

static parse_t parse_and( seq_t in, any args ) {

  int n = *(int*)args;
  parser_p* parsers = (parser_p*)( ((int*)args) + 1 );

  parse_t result; result.rest = in;
  for( int i=0; i<n; i++ ) {

    parser_p inferior_parser = parsers[i];
    result = inferior_parser->parse( result.rest, inferior_parser->args );

    if( FULL_PARSE != result.status )
      break;

  }

  return result;
}

static parse_t parse_or( seq_t in, any args ) {
  
  int n = *(int*)args;
  parser_p* parsers = (parser_p*)( ((int*)args) + 1 );

  parse_t result; result.rest = in;
  for( int i=0; i<n; i++ ) {

    parser_p inferior_parser = parsers[i];

    result = inferior_parser->parse( in, inferior_parser->args );
    if( FAIL_PARSE != result.status )
      return result;

  }

  return (parse_t){ .status = FAIL_PARSE,
      .rest = in
      };

}

// Parser ctors

#define defparser( name, argslist, argexpr )				\
  parser_p name##_PARSE( any ctx, parse_action_f action, argslist ) {	\
parser_p parser = new(ctx, parser_t);			\
							\
parser->parse = parse_##name;				\
parser->args = argexpr;					\
parser->action = action;				\
							\
return parser;						\
}

#define defparser0( name )						\
  parser_p name##_PARSE( any ctx, parse_action_f action ) {	\
parser_p parser = new(ctx, parser_t);			\
							\
parser->parse = parse_##name;				\
parser->args = NULL;					\
parser->action = action;				\
							\
return parser;						\
}

defparser( char, char c, (any)(int)c );
defparser( charset, const char* charset, clone_string(parser,charset) );
defparser( string, const char* s, clone_string(parser,s) );
defparser0( long );
defparser0( longlong );
defparser0( float );
defparser0( double );
defparser0( longdouble );

defparser( maybe, parser_p inferior_parser, inferior_parser );
defparser( many0, parser_p inferior_parser, inferior_parser );
defparser( many1, parser_p inferior_parser, inferior_parser );

parser_p and_PARSE( any ctx, parse_action_f action, int n, ... ) {
  
  parser_p parser = new(ctx, parser_t);

  parser->parse = parse_and;
  parser->args = alloc(parser, sizeof(int) + n * sizeof(parser_p));

  *(int*)parser->args = n;
  parser_p* parsers = (parser_p*)( ((int*)parser->args)+1 );

  va_list args; va_start(args, n);
  for( int i=0; i<n; i++ )
    parsers[i] = va_arg(args, parser_p);
  va_end(args);

  parser->action = action;

  return parser;

}

parser_p or_PARSE( any ctx, parse_action_f action, int n, ... ) {
  
  parser_p parser = new(ctx, parser_t);

  parser->parse = parse_or;
  parser->args = alloc(parser, sizeof(int) + n * sizeof(parser_p));

  *(int*)parser->args = n;
  parser_p* parsers = (parser_p*)( ((int*)parser->args)+1 );

  va_list args; va_start(args, n);
  for( int i=0; i<n; i++ )
    parsers[i] = va_arg(args, parser_p);
  va_end(args);

  parser->action = action;

  return parser;

}
