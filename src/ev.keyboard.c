#include <assert.h>
#include <SDL_events.h>
#include <string.h>
#include "core.string.h"
#include "ev.keyboard.h"

//#define sdl_ev_mask SDL_KEYEVENTMASK

static uint8 init_kbd_EV( enable_ev_f enable,
                          disable_ev_f disable,
                          va_list args ) {
	
	enable( SDL_KEYDOWN );
	enable( SDL_KEYUP );
	
	return 0;
//	return sdl_ev_mask;

}

static int translate_kbd_EV( ev_t* dest, const SDL_Event* ev ) {

//	assert( 0 != (SDL_EVENTMASK(ev->type) & sdl_ev_mask) );

	dest->kbd.pressed = (ev->key.state == SDL_PRESSED);
	dest->kbd.key = (uint16)ev->key.keysym.sym;
	dest->kbd.ch = (wchar_t)ev->key.keysym.unicode;
	dest->kbd.modifiers = ev->key.keysym.mod;

	return 0;

}

static int describe_kbd_EV( ev_t* ev, int n, char* dest ) {
	
	const char* name = SDL_GetKeyName( (SDLKey)ev->kbd.key );
	return maybe_strncpy( dest, n, name );

}

static int detail_kbd_EV( ev_t* ev, int n, char* dest ) {

	char         buf[4092];
	char*      state = (ev->kbd.pressed ? "Pressed" : "Released");
	const char* name = SDL_GetKeyName( (SDLKey)ev->kbd.key );

	// Pressed|Released <key> key
	sprintf(buf, "%s `%s` key", state, name);
	return maybe_strncpy( dest, n, buf );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evKeyboard,
	.ev_size      = sizeof(ev_kbd_t),
//	.ev_mask      = sdl_ev_mask,

	.init_ev      = init_kbd_EV,
	.translate_ev = translate_kbd_EV,
	.describe_ev  = describe_kbd_EV,
	.detail_ev    = detail_kbd_EV

};
ev_adaptor_p       kbd_EV_adaptor = &adaptor;
