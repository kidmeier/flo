#ifndef __seq_core_h__
#define __seq_core_h__

#include <stdarg.h>
#include <stdlib.h>
#include "core.types.h"

/* A seqable implements the seq interface over some underlying data. */
struct seqable_s;
typedef struct seqable_s seqable_t;
typedef seqable_t* seqable_p;

/* 
   The seq structure. A seq needs to know which seqable implements its underlying abstraction,
   sequence is a pointer to the underlying data, and first points to the current 'first' element
   of the seq. An implementation is free to use sequence and first in whatever way they please,
   they are not touched by the SEQ machinery.

   flags is a bitfield carrying information specific to this seq_t. Currently:
    SEQ_OWNER if this seq_t owns the underlying storage
    SEQ_NIL   if this is the end of the seq. this implies first is NULL, and calls to first will
               return NULL
*/
struct seq_s {
  seqable_p seqable;
  pointer   sequence;
  pointer   first;

  uint8     flags;
};
typedef struct seq_s seq_t;

/* Core seq_t interface */
pointer first_SEQ( const seq_t* seq );
seq_t   next_SEQ( const seq_t* seq );
seq_t   rest_SEQ( const seq_t* seq );

seq_t   seq_SEQ( const seqable_p seqable, pointer sequence, va_list args );
void    unseq_SEQ( const seq_t* seq );

// Seq flags
#define OWNER_SEQF    0x01
#define CASCADE_SEQF  0x02
#define NIL_SEQF      0x80

/* 
   Inline convenience wrappers around SEQ_*. It is intended that clients use these
   functions rather than SEQ_* directly.
*/
static inline seq_t seq(const seqable_p seqable, pointer sequence, ...) {
  va_list args;

  va_start(args,sequence);
  seq_t sq = seq_SEQ( seqable, sequence, args );
  va_end(args);

  return sq;
}

static inline void unseq(seq_t seq) {
  unseq_SEQ( &seq );
}

static inline pointer first(seq_t sq) {
  return first_SEQ( &sq );
}

static inline seq_t next(seq_t sq) {
  return next_SEQ( &sq );
}

static inline seq_t rest(seq_t sq) {
  return rest_SEQ( &sq );
}

static inline bool owner(seq_t sq) {
  return 0 != (sq.flags & OWNER_SEQF);
}

static inline bool nil(seq_t sq) {
  return 0 != (sq.flags & NIL_SEQF);
}

seq_t nilify_SEQ( const seq_t* sq );
extern seq_t nil_SEQ;

/*
  This the seqable interface. The only thing that needs to be exposed to
  users is a seqable_p. The rest of the machinery is taken care of.
*/
struct seqable_s {

  seq_t   (*seq)(const seqable_p self, pointer sequence, va_list args);
  void    (*unseq)(const seq_t* seq);
  pointer (*first)(const seq_t* seq);
  seq_t   (*rest)(const seq_t* seq);
  seq_t   (*next)(const seq_t* seq);

};

/*
  Intended for use by seqable implementations. symbol should be an identifier declared
  as type seqable_p and exposed to clients through a header. The rest of the arguments
  should be the names of functions implementing the seqable interface.
*/
#define declare_SEQABLE( symbol )			 \
  extern const seqable_p symbol##_SEQABLE

#define ref_SEQABLE( symbol ) \
  symbol##_SEQABLE

#define def_SEQABLE( symbol )				 \
  static seqable_t symbol##_SEQABLE_t = {		 \
    symbol##_seq,					 \
    symbol##_unseq,					 \
    symbol##_first,					 \
    symbol##_rest,					 \
    symbol##_next					 \
  };							 \
  const seqable_p symbol##_SEQABLE = &symbol##_SEQABLE_t

/* SEQ-based algorithms/transformers */
pointer reduce( seq_t sq, pointer seed, pointer (*f)(pointer, pointer) );
seq_t   take( int n, seq_t sq );
seq_t   drop( int n, seq_t sq );
pointer nth( int n, seq_t sq );
seq_t   cycle( seq_t src );
seq_t   zip( seq_t a, seq_t b );
seq_t   interpose( pointer sep, seq_t sq );

#endif
