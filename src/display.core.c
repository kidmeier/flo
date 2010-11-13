#include <stdarg.h>

#include <SDL.h>
#include "display.core.h"

static SDL_Surface* display = NULL;

static void maybe_initialize_video(void) {

  if( SDL_INIT_VIDEO != SDL_WasInit(SDL_INIT_VIDEO) )
    SDL_InitSubSystem( SDL_INIT_VIDEO );

}

int set_DISPLAY( int width, int height, int bpp, int flags, ... ) {

  va_list attribs;

  maybe_initialize_video();

  va_start(attribs, flags);
  while( true ) {

    int attrib = va_arg(attribs, int);
    if( attrib < 0 )
      break;

    int value = va_arg(attribs, int);
    
    SDL_GL_SetAttribute( attrib, value );

  }
  va_end(attribs);

  display = SDL_SetVideoMode( width, height, bpp, flags | SDL_OPENGL );
  if( !display )
    return -1;

  return 0;
}

int check_DISPLAY( int width, int height, int bpp, int flags ) {

  maybe_initialize_video();
  return SDL_VideoModeOK( width, height, bpp, flags | SDL_OPENGL );

}
