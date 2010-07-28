#include <assert.h>
#include <stdbool.h>
#include "core.alloc.h"
#include "seq.filter.h"

typedef bool (*predicate_f)( pointer val );

struct filter {

  predicate_f predicate;
  seq_t (*step)(const seq_t* seq);

};

/*
  Steps the inferior_seq until it satisfies predicate and
  returns the first seq_t which satisfies the predicate.
*/
static seq_t* advance_inferior_seq( struct filter* filt, seq_t* inferior_seq ) {

  while( !filt->predicate(first_SEQ(inferior_seq)) ) {
    *inferior_seq = filt->step( inferior_seq );
    if( nil(*inferior_seq) )
      return inferior_seq;
  }

  return inferior_seq;
}

static seq_t filter_seq( const seqable_p self, pointer sequence, va_list args ) {
  
  struct filter* filt = new( NULL, struct filter );

  filt->predicate = va_arg(args, predicate_f);

  int how = va_arg(args, int);
  filt->step = (how == NEXT_FILTER_SEQ) ? next_SEQ : rest_SEQ;

  seq_t* inferior_seq = clone_into(filt,seq_t,sequence);
  advance_inferior_seq( filt, inferior_seq );

  return (seq_t){ self, filt, inferior_seq, inferior_seq->flags | OWNER_SEQF };

}

static void filter_unseq( const seq_t* seq ) {

  seq_t* inferior_seq = (seq_t*)seq->first;
  unseq_SEQ(inferior_seq);

  delete( seq->sequence );

}

static pointer filter_first( const seq_t* seq ) {

  seq_t* inferior_seq = (seq_t*)seq->first;
  return first_SEQ(inferior_seq);

}

static seq_t filter_rest( const seq_t* seq ) { 

  struct filter* filt = (struct filter*)seq->sequence;
  seq_t* inferior_seq = clone_into(seq->sequence, seq_t, seq->first);

  *inferior_seq = filt->step( inferior_seq );
  advance_inferior_seq( filt, inferior_seq);
  return (seq_t){ seq->seqable, seq->sequence, inferior_seq, 
      (inferior_seq->flags | seq->flags) & ~(OWNER_SEQF) };
}

static seq_t filter_next( const seq_t* seq) {

  struct filter* filt = (struct filter*)seq->sequence;
  seq_t* inferior_seq = (seq_t*)seq->first;

  *inferior_seq = filt->step( inferior_seq );
  advance_inferior_seq( filt, inferior_seq );
  return (seq_t){ seq->seqable, seq->sequence, inferior_seq, inferior_seq->flags | seq->flags };
}

def_SEQABLE( filter );

// Public access
seq_t filter( seq_t sq, bool (*pred)(pointer), int how ) {

  return seq( ref_SEQABLE(filter), &sq, pred, how );

}

#ifdef __seq_filter_TEST__

#include <stdio.h>
#include "seq.array.h"

static bool negative(pointer val) {
  
  int x = *(int*)val;
  if( x < 0 )
    return true;
  else
    return false;

}

int main(int argc, char* argv[] ) {

  int Z[] = { -5, 0, 10, -20, -1, 5, 6, 7 };
  seq_t negatives = filter( array(Z, sizeof(Z)/sizeof(Z[0]), sizeof(int)), negative, NEXT_FILTER_SEQ );

  while( !nil(negatives) ) {

    printf("%d ", *(int*)first(negatives));
    negatives = next(negatives);

  }
  printf("\n");

  unseq(negatives);
  return 0;
}

#endif
