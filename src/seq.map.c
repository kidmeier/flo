#include "core.alloc.h"
#include "seq.map.h"

typedef pointer (*map_f)( pointer val );

struct map {

  map_f func;

};

static seq_t map_seq( const seqable_p self, pointer inferior_seq, va_list args) {

  struct map* m = new( NULL, struct map );

  m->func = va_arg(args, map_f);
  return (seq_t){ self, m, clone_into(m, seq_t, inferior_seq), OWNER_SEQF };

}

static void map_unseq( const seq_t* sq ) {

  seq_t* inferior_seq = (seq_t*)sq->first;
  unseq_SEQ(inferior_seq);
  
  delete(sq->sequence);

}

static pointer map_first( const seq_t* sq ) {

  struct map* m = (struct map*)sq->sequence;
  return m->func( first_SEQ( (const seq_t*)sq->first) );

}

static seq_t map_rest( const seq_t* sq ) {

  seq_t* inferior_seq = clone_into( sq->sequence, seq_t, sq->first );

  *inferior_seq = rest_SEQ( inferior_seq );
  return (seq_t){ 
    sq->seqable, 
      sq->sequence, 
      inferior_seq, 
      (sq->flags | inferior_seq->flags) & ~(OWNER_SEQF)
      };

}

static seq_t map_next( const seq_t* sq ) {

  seq_t* inferior_seq = (seq_t*)sq->first;

  *inferior_seq = next_SEQ( inferior_seq );

  return (seq_t) { 
    sq->seqable, 
      sq->sequence, 
      inferior_seq, 
      sq->flags | inferior_seq->flags
      };
}

def_SEQABLE( map );

seq_t map( map_f f, seq_t sq ) {

  return seq( ref_SEQABLE(map), &sq, f );

}

#ifdef __seq_map_TEST__

#include <stdio.h>
#include "seq.array.h"

pointer square( pointer val ) {
  
  int x = *(int*)val;
  return (pointer)( x * x);

}
  
int main(int argc, char* argv[]) {

  int N[] = { 1, 2, 3, 4, 5 };
  seq_t squares = map( square, array(N, 5, sizeof(int)) );

  while( !nil(squares) ) {

    int x = (int)first(squares);
    printf("%d ", x);
    squares = next(squares);
  }
  printf("\n");

  unseq(squares);
  return 0;
}

#endif
