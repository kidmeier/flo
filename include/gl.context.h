#ifndef __gl_context_h__
#define __gl_context_h__

#include <SDL_video.h>

#include "gl.display.h"

typedef SDL_GLContext Glcontext;

Glcontext create_Glcontext( Display* );
void      delete_Glcontext( Glcontext );
int         bind_Glcontext( Display*, Glcontext );

#endif
