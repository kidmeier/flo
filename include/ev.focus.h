#ifndef __ev_focus_h__
#define __ev_focus_h__

#include "ev.core.h"

#define focusMouse     0x01
#define focusKeyboard  0x02
#define focusMinimized 0x04

// WARNING: This is not re-entrant; should only be called from one thread.
int init_focus_EV( ev_t* dest, const union SDL_Event* ev );
int describe_focus_EV( ev_t* ev, int n, char* dest );
int detail_focus_EV( ev_t* ev, int n, char* dest );

#endif
