#include "res.md5.h"
#include "seq.array.h"
#include "seq.file.h"
#include "math.vec.h"

static float4 quat( float x, float y, float z ) {
  float t = 1.0 - (x*x) - (y*y) - (z*z);
  if( t < 0.0f )
    t = 0.0f;
  else
    t = -sqrt(t);

  return (float4){ x, y, z, t };
}

static 
md5mesh_p parse_header( seq_t input ) {

  static parser_p parser = NULL;
  if( !parser ) {
    
  }

}

md5mesh_p parse_mesh_MD5( const char* path ) {

  seq_t in = read_FILE_SEQ(path, SHARED_FILE_SEQ);
  if( nil(in) )
    return NULL;

  md5mesh_p mesh = parse_header(in);

}
