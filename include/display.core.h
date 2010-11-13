#ifndef __display_core_h__
#define __display_core_h__

#include <SDL_video.h>
#include "core.types.h"

enum display_attrib_e {

  redBits = SDL_GL_RED_SIZE,
  greenBits = SDL_GL_GREEN_SIZE,
  blueBits = SDL_GL_BLUE_SIZE,
  alphaBits = SDL_GL_ALPHA_SIZE,

  depthBits = SDL_GL_DEPTH_SIZE,
  frameBufferBits = SDL_GL_BUFFER_SIZE,
  stencilBits = SDL_GL_STENCIL_SIZE,

  accRedBits = SDL_GL_ACCUM_RED_SIZE,
  accGreenBits = SDL_GL_ACCUM_GREEN_SIZE,
  accBlueBits = SDL_GL_ACCUM_BLUE_SIZE,
  accAlphaBits = SDL_GL_ACCUM_ALPHA_SIZE,
  
  doubleBuffer = SDL_GL_DOUBLEBUFFER,
  stereo = SDL_GL_STEREO,
  multiSampleBuffers = SDL_GL_MULTISAMPLEBUFFERS,
  multiSampleSamples = SDL_GL_MULTISAMPLESAMPLES,

//  vsync = SDL_GL_SWAP_CONTROL,

  requireAccel = SDL_GL_ACCELERATED_VISUAL

};  

enum window_flags_e {

  fullscreen = SDL_FULLSCREEN,
  resizable = SDL_RESIZABLE,
  noframe = SDL_NOFRAME

};

int set_DISPLAY( int width, int height, int bpp, int flags, ... );
int check_DISPLAY( int width, int height, int bpp, int flags );

#endif
