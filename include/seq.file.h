#ifndef __seq_file_h__
#define __seq_file_h__

#include "seq.core.h"
declare_SEQABLE( file );

#include <sys/mman.h>

#define SHARED_FILE_SEQ     MAP_SHARED
#define PRIVATE_FILE_SEQ    MAP_PRIVATE

seq_t   read_FILE_SEQ( const char* pathname, int sharelevel );
void    close_FILE_SEQ( seq_t seq );
size_t  length_FILE_SEQ( seq_t seq );

#endif
