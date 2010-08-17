#ifndef __ev_keyboard_h__
#define __ev_keyboard_h__

union SDL_Event;

#include "ev.core.h"

enum kbd_modifiers_e {

	// Use the same modifiers as SDL
	modLshift = 0x0001,
	modRshift = 0x0002,
	modLctrl  = 0x0040,
	modRctrl  = 0x0080,
	modLalt   = 0x0100,
	modRalt   = 0x0200,
	modLmeta  = 0x0400,
	modRmeta  = 0x0800,
	modNum    = 0x1000,
	modCaps   = 0x2000,
	modMode   = 0x4000

};

extern ev_adaptor_p kbd_EV_adaptor;

#endif
