#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "core.alloc.h"
#include "seq.file.h"

struct file {

  int   fd;
  void* base;
  size_t length;

  int prot;
  int sharelevel;

};

static seq_t file_seq( const seqable_p self, pointer sequence, va_list args ) {

  const char* pathname = (const char*)sequence;
  int fd = open( pathname, O_RDONLY );
  if( fd < 0 ) {
    return nil_SEQ;
  }

  struct stat st;
  if( fstat( fd, &st ) < 0 ) {
    close(fd);
    return nil_SEQ;
  }

  struct file* file = new( NULL, struct file );
  file->fd = fd;
  file->prot = va_arg(args, int);
  file->sharelevel = va_arg(args, int);
  file->length = st.st_size;
  file->base = mmap( NULL, file->length, file->prot, file->sharelevel, file->fd, 0 );

  if( MAP_FAILED == file->base ) {
    free(file);
    close(fd);
    
    return nil_SEQ;
  }

  return (seq_t){ self, file, file->base, OWNER_SEQF };
}

static void file_unseq( const seq_t* seq ) {

  // Unmap the file and then close it
  struct file* file = (struct file*)seq->sequence;
  munmap( file->base, file->length );
  close( file->fd );
  
  // Free the struct
  delete( file );
}

static pointer file_first( const seq_t* seq ) {
  return seq->first;
}

static inline bool eof( pointer first, struct file* f ) {

  if( first - f->base >= f->length )
    return true;

  return false;
}

static seq_t file_rest( const seq_t* seq ) {

  struct file* f = (struct file*)seq->sequence;

  if( eof( seq->first + 1, f ) )
    return nilify_SEQ(seq);

  seq_t the_rest = { seq->seqable, seq->sequence, seq->first + 1, seq->flags };
  return the_rest;

}

static seq_t file_next( const seq_t* seq ) {

  struct file* f = (struct file*)seq->sequence;

  if( eof( seq->first + 1, f ) )
    return nilify_SEQ(seq);

  return (seq_t){ seq->seqable, seq->sequence, seq->first + 1, seq->flags | OWNER_SEQF };

}

def_SEQABLE( file );

// Client API /////////////////////////////////////////////////////////////////

seq_t   read_FILE_SEQ( const char* pathname, int sharelevel ) {
  return seq( ref_SEQABLE(file), (pointer)pathname, PROT_READ, sharelevel );
}

void close_FILE_SEQ( seq_t seq ) {

  assert( ref_SEQABLE(file) == seq.seqable );
  assert( owner(seq) );
  unseq( seq );

}

size_t  length_FILE_SEQ( seq_t seq ) {
  
  assert( ref_SEQABLE(file) == seq.seqable );
  
  struct file* file = (struct file*)seq.sequence;
  return file->length;
}

#ifdef __seq_file_TEST__

#include <stdio.h>

int main( int argc, char* argv[] ) {

  seq_t bytes = read_FILE_SEQ(argv[1], SHARED_FILE_SEQ);
  int count = 0;
  while( !nil(bytes) ) {

    printf("%c", *(char*)first(bytes));
    bytes = rest(bytes);

    count++;
  }

  printf("\nlength: %d bytes\n", length_FILE_SEQ(bytes));
  printf("count:  %d bytes\n", count);

  unseq(bytes);
  return 0;
}

#endif
