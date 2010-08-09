#ifndef __ev_core_h__
#define __ev_core_h__

#include "core.types.h"
#include "time.core.h"

// Forward decl
union SDL_Event;

// Event types

enum ev_type_e {

	evUnknown = -1,

	evKeyboard,
	evMouse,
	evJoystick,

	evFocus,
	evWindow,
	evPlatform,    // Platform specific events from window manager

	evQuit,

	evTypeCount

};

typedef struct ev_info_s {

	msec_t         time;
	enum ev_type_e type;

} ev_info_t;

typedef struct ev_kb_s {

	ev_info_t info;

	bool      pressed;

	uint16    key;
	wchar_t   ch;
	
	uint16    modifiers;
	
} ev_kb_t;

typedef struct ev_axis_s {

	uint16 ord;
	int16  delta;

} ev_axis_t;

typedef struct ev_mouse_s {

	ev_info_t info;

	ev_axis_t X;
	ev_axis_t Y;
	ev_axis_t Z;

	uint16    buttons;
	uint16    prev_buttons;

} ev_mouse_t;

typedef union ev_u {
	
	ev_info_t  info;
	ev_kb_t    kbd;
	ev_mouse_t mouse;
	
} ev_t;

struct ev_channel_s;

int           init_EV( void );
int           pump_EV( void );
struct
ev_channel_s* open_EV( enum ev_type_e type );

#endif
