#ifndef __display_core_h__
#define __display_core_h__

#include <SDL_video.h>
#include "core.types.h"

typedef enum {

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

        requireAccel = SDL_GL_ACCELERATED_VISUAL,

             glMajor = SDL_GL_CONTEXT_MAJOR_VERSION,
             glMinor = SDL_GL_CONTEXT_MINOR_VERSION,

} display_attrib_e;

typedef enum {

  fullscreen = SDL_WINDOW_FULLSCREEN,
  resizable  = SDL_WINDOW_RESIZABLE,
  noframe    = SDL_WINDOW_BORDERLESS

} display_flags_e;

typedef SDL_Window Display;

Display*  open_Display( const char*, int width, int height, int flags, ... );
void     close_Display( Display* dpy );
void      flip_Display( Display* );

#endif
