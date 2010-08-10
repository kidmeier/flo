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

// Base event info; all event structures must begin with this
typedef struct ev_info_s {

	usec_t         time;
	enum ev_type_e type;

} ev_info_t;

// Keyboard
typedef struct ev_kb_s {

	ev_info_t info;

	bool      pressed;

	uint16    key;
	wchar_t   ch;
	
	uint16    modifiers;
	
} ev_kb_t;

// Axis; used by mouse and joystick
typedef struct ev_axis_s {

	uint16 ord;
	int16  delta;

} ev_axis_t;

// Mouse
typedef struct ev_mouse_s {

	ev_info_t info;

	ev_axis_t X;
	ev_axis_t Y;
	ev_axis_t Z;

	uint16    buttons;
	uint16    prev_buttons;

} ev_mouse_t;

// Joystick TODO

// Focus
typedef struct ev_focus_s {

	ev_info_t info;
	uint8     state;

} ev_focus_t;

// Window
typedef struct ev_window_s {

	ev_info_t info;

	uint8     exposed;

	uint16    width;
	uint16    height;

} ev_window_t;

// Platform TODO (WM-specific events)

// Quit
typedef struct ev_quit_s {

	ev_info_t info;

} ev_quit_t;	

// Event union
typedef union ev_u {
	
	ev_info_t   info;

	ev_kb_t     kbd;
	ev_mouse_t  mouse;

	ev_focus_t  focus;
	ev_window_t window;

	ev_quit_t   quit;
	
} ev_t;

struct ev_channel_s;

int           init_EV( void );
int           pump_EV( void );
bool          quit_requested_EV( void );
struct
ev_channel_s* open_EV( enum ev_type_e type );

#endif
