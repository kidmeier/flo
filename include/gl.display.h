#ifndef __display_core_h__
#define __display_core_h__

#include <SDL_video.h>
#include "core.types.h"

typedef enum {

  fullscreenDisplay = SDL_WINDOW_FULLSCREEN,
  resizableDisplay  = SDL_WINDOW_RESIZABLE,
  noframeDisplay    = SDL_WINDOW_BORDERLESS,

} display_flags_e;

typedef SDL_Window Display;

Display *open_Display( const char*, 
                       int width, int height, int flags, 
                       int red, int blue, int green, int alpha,
                       int depth, int stencil,
                       int glMajor, int glMinor );

void    close_Display( Display* dpy );
void     flip_Display( Display* );
void   resize_Display( Display *dpy, int width, int height );

float  aspect_Display( Display* dpy );
int     width_Display( Display* dpy );
int    height_Display( Display* dpy );

#endif
