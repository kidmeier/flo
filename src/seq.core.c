#include <assert.h>
#include "seq.core.h"

seq_t nil_SEQ = { NULL, NULL, NULL, NIL_SEQF };

seq_t nilify_SEQ( const seq_t* sq ) {

  return (seq_t){ sq->seqable, sq->sequence, NULL, sq->flags | NIL_SEQF };

}

seq_t seq_SEQ( const seqable_p seqable, any sequence, va_list args ) {

  return seqable->seq( seqable, sequence, args );

}

void  unseq_SEQ( const seq_t* sq ) {

  assert( owner(*sq) );

  const seqable_p seqable = sq->seqable;
  seqable->unseq( sq );

}

any  first_SEQ( const seq_t* seq ) {

  const seqable_p seqable = seq->seqable;
  return seqable->first( seq );
}

seq_t rest_SEQ( const seq_t* seq ) {
  
  const seqable_p seqable = seq->seqable;
  return seqable->rest( seq );
}

seq_t next_SEQ( const seq_t* seq ) {

  assert( owner(*seq) );
  
  const seqable_p seqable = seq->seqable;
  return seqable->next( seq );
}

// SEQ-based algorithms

any   reduce( seq_t sq, any seed, any (*f)(any, any) ) {

  return NULL;

}

seq_t take( int n, seq_t sq ) {

  return nil_SEQ;

}

seq_t drop( int n, seq_t sq ) {

  return nil_SEQ;

}

any   nth( int n, seq_t sq ) {

  return NULL;

}

seq_t cycle( seq_t src ) {

  return nil_SEQ;

}

seq_t zip( seq_t a, seq_t b ) {

  return nil_SEQ;

}

seq_t interpose( any sep, seq_t sq ) {

  return nil_SEQ;

}
