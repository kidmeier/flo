#include <assert.h>
#include <SDL/SDL_events.h>
#include <string.h>
#include "core.string.h"
#include "ev.keyboard.h"

int init_kbd_EV( ev_t* dest, const SDL_Event* ev ) {

	assert( 0 != (SDL_EVENTMASK(ev->type) & SDL_KEYEVENTMASK) );

	dest->kbd.pressed = (ev->key.state == SDL_PRESSED);
	dest->kbd.key = (uint16)ev->key.keysym.sym;
	dest->kbd.ch = (wchar_t)ev->key.keysym.unicode;
	dest->kbd.modifiers = ev->key.keysym.mod;

	return 0;

}

int describe_kbd_EV( ev_t* ev, int n, char* dest ) {
	
	const char* name = SDL_GetKeyName( (SDLKey)ev->kbd.key );
	return maybe_strncpy( dest, n, name );

}

int detail_kbd_EV( ev_t* ev, int n, char* dest ) {

	char  buf[4092];
	char* state = (ev->kbd.pressed ? "Pressed" : "Released");
	char* name = SDL_GetKeyName( (SDLKey)ev->kbd.key );

	// ss.ssss s Pressed|Released <key> key
	sprintf(buf, "% 8.3fs %s `%s` key", (double)ev->info.time / 1000.0, state, name);
	
	return maybe_strncpy( dest, n, buf );

}
