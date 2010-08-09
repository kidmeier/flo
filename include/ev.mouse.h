#ifndef __ev_mouse_h__
#define __ev_mouse_h__

union SDL_Event;

#include "ev.core.h"

#define mouseButton(n)                (1 << ((n)-1))
#define isMouseButtonDown(buttons,n) ( 0 != ((buttons) & mouseButton( (n) )) )
#define isMouseButtonUp(buttons,n)   !isMouseButtonDown( (n) )
#define isMouseButtonsChanged(evp)    ( (evp)->buttons != (evp)->prev_buttons )
#define mouseButtonsDelta(evp)        ( (evp)->buttons ^ (evp)->prev_buttons )

#define mouseButtonsCount 16

// WARNING: This is not re-entrant; should only be called from one thread.
int init_mouse_EV( ev_t* dest, const union SDL_Event* ev );
int describe_mouse_EV( ev_t* ev, int n, char* dest );
int detail_mouse_EV( ev_t* ev, int n, char* dest );

#endif
