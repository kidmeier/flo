#ifndef __ev_window_h__
#define __ev_window_h__

#include "ev.core.h"

// WARNING: This is not re-entrant; should only be called from one thread.
int init_window_EV( ev_t* dest, const union SDL_Event* ev );
int describe_window_EV( ev_t* ev, int n, char* dest );
int detail_window_EV( ev_t* ev, int n, char* dest );

#endif
