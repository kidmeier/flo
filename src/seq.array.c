#include <assert.h>
#include <stdbool.h>
#include "core.alloc.h"
#include "seq.array.h"

// ARRAY seqable //////////////////////////////////////////////////////////////

struct array {
  pointer base;
  uint    size;
  uint    stride;
};

static seq_t array_seq( const seqable_p self, pointer first, va_list args ) {
  
  uint size = va_arg(args, uint);
  uint stride = va_arg(args, uint);

  // Makes no sense otherwise
  assert( size % stride == 0 );

  struct array* ary = new( NULL, struct array );
  
  ary->base = first;
  ary->size = size;
  ary->stride = stride;

  return (seq_t){ self, ary, first, OWNER_SEQF };
}

static void array_unseq( const seq_t* seq ) {

  // Free the underlying array and the seq-array
  if( seq->flags & CASCADE_SEQF ) {
    struct array* ary = (struct array*)seq->sequence;
    delete(ary->base);
  }

  delete(seq->sequence);
}

static pointer array_first( const seq_t* seq ) {
  return seq->first;
}

static bool out_of_bounds( pointer first, struct array* ary ) {

  if( first >= ary->base + ary->size ) {
    return true;
  }

  return false;

} 

static seq_t array_rest( const seq_t* seq ) {

  struct array* ary = (struct array*)seq->sequence;
  if( out_of_bounds( seq->first + ary->stride, ary ) )
    return nilify_SEQ(seq);

  return (seq_t)
    { seq->seqable, seq->sequence, seq->first + ary->stride, seq->flags & ~(OWNER_SEQF) };
}

static seq_t array_next( const seq_t* seq ) {

  struct array* ary = (struct array*)seq->sequence;
  if( out_of_bounds( seq->first + ary->stride, ary ) )
    return nilify_SEQ(seq);

  return (seq_t){ seq->seqable, seq->sequence, seq->first + ary->stride, seq->flags };
}

// Export the interface
def_SEQABLE( array );

seq_t array( pointer ary, int n, int stride ) {

  return seq( ref_SEQABLE(array), ary, n*stride, stride );

}

#ifdef __seq_array_TEST__

#include <stdio.h>

int main(int argc, char* argv[]) {

  int N[] = { 0, 1, 2, 3, 4, 5 };
  const char* NL[] = { "zero", "one", "two", "three", "four", "five" };

  seq_t ns = array(N, sizeof(N)/sizeof(N[0]), sizeof(N[0]));
  seq_t nls = array(NL, sizeof(NL)/sizeof(NL[0]), sizeof(NL[0]));

  while( !nil(ns) && !nil(nls) ) {

    int n = *(int*)first(ns);
    const char* nl = *(const char**)first(nls);

    printf("%d = %s\n", n, nl);

    ns = next(ns); nls = next(nls);
  }
  unseq(ns); unseq(nls);

  return 0;
}

#endif
