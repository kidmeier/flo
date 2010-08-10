#ifndef __ev_quit_h__
#define __ev_quit_h__

#include "ev.core.h"

int init_quit_EV( ev_t* dest, const union SDL_Event* ev );
int describe_quit_EV( ev_t* ev, int n, char* dest );
int detail_quit_EV( ev_t* ev, int n, char* dest );

#endif
