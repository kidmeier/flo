#ifndef __gl_context_headless_H__
#define __gl_context_headless_H__

#include "core.features.h"

#if defined( feature_X11 ) && defined( feature_GLX )

#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#else

#error Not supported on this platform.

#endif

#include "core.types.h"

typedef GLXContext Glcontext_headless;

Glcontext_headless create_Glcontext_headless( void );
void              destroy_Glcontext_headless( Glcontext_headless glc );

#endif
