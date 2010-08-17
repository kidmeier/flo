#ifndef __ev_core_h__
#define __ev_core_h__

#include <stdarg.h>

#include "data.bitset.h"
#include "core.types.h"
#include "time.core.h"

// Forward decl
union SDL_Event;

// Event types

enum ev_type_e {

	evUnknown = -1,

	// From input devices
	evKeyboard,
	evButton,
	evAxis,
	evCursor,
	evDpad,

	evFocus,
	evWindow,
	evPlatform,    // Platform specific events from window manager

	evQuit,

	evTypeCount

};

// Base event info; all event structures must begin with this
typedef struct ev_info_s {

	usec_t         time;
	uint32         tick;
	enum ev_type_e type;

} ev_info_t;

// Keyboard
typedef struct ev_kbd_s {

	ev_info_t info;

	bool      pressed;

	uint16    key;
	wchar_t   ch;
	
	uint16    modifiers;
	
} ev_kbd_t;

// Axis; from a joystick
typedef struct ev_axis_s {

	ev_info_t info;

	uint8     which;

	uint16    ord;
	int16     delta;

} ev_axis_t;

// Cursor; mouse or joystick trackball
typedef struct ev_cursor_s {

	ev_info_t info;
	
	uint8     which;
   
	uint16    X,  Y;
	int16    dX, dY;
	
} ev_cursor_t;

// Button; from a mouse or joystick
#define maxButtonCount 256
typedef struct ev_button_s {

	ev_info_t info;

	uint8     which;
	uint8     pressed;

	// Button state is just a massive bitset
	bitset    (state, maxButtonCount);

} ev_button_t;

// Dpad; a hat, or a traditional 'dpad' on a joystick
typedef struct ev_dpad_s {

	ev_info_t info;

	uint8     which;
	uint8     pos;

} ev_dpad_t;

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

	ev_axis_t   axis;
	ev_button_t button;
	ev_cursor_t cursor;
	ev_dpad_t   dpad;
	ev_kbd_t    kbd;

	ev_focus_t  focus;
	ev_window_t window;

	ev_quit_t   quit;
	
} ev_t;

// Device adaptor
typedef struct ev_adaptor_s {

	enum ev_type_e ev_type;
	uint16         ev_size;

	uint32         ev_mask;

	// Receives arguments passed to open_EV in va_list
	// Return value is an SDL event filter mask to enable requisite events
	uint32 (*init_ev)( va_list );
	int    (*translate_ev)( ev_t*, const union SDL_Event* );
	int    (*describe_ev)( ev_t*, int, char* );
	int    (*detail_ev)( ev_t*, int, char* );

} ev_adaptor_t;
typedef ev_adaptor_t* ev_adaptor_p;

// Forward decl.
struct ev_channel_s;

int           init_EV( void );
int           pump_EV( uint32 );
bool          quit_requested_EV( void );
struct
ev_channel_s* open_EV( ev_adaptor_p, ... );

#endif
