#include <assert.h>
#include <SDL/SDL_events.h>

#include "core.string.h"
#include "ev.focus.h"

#define sdl_ev_mask SDL_ACTIVEEVENTMASK

static uint32 init_focus_EV( va_list args ) {

	return sdl_ev_mask;

}

// WARNING: This is not re-entrant; should only be called from one thread.
static int translate_focus_EV( ev_t* dest, const union SDL_Event* ev ) {

	static uint8 focus = 0;

	assert( 0 != (SDL_EVENTMASK(ev->type) & sdl_ev_mask) );

	if( ev->active.gain ) {
		
		focus = (0 != (ev->active.state & SDL_APPMOUSEFOCUS)) 
			? ( focus | focusMouse ) 
			: focus;
		focus = (0 != (ev->active.state & SDL_APPINPUTFOCUS))
			? ( focus | focusKeyboard )
			: focus;
		focus = (0 != (ev->active.state & SDL_APPACTIVE))
			? (focus & ~(focusMinimized))
				: focus;
		
	} else {
		
		focus = (0 != (ev->active.state & SDL_APPMOUSEFOCUS)) 
			? ( focus & ~(focusMouse) ) 
			: focus;
		focus = (0 != (ev->active.state & SDL_APPINPUTFOCUS))
			? ( focus & ~(focusKeyboard) )
				: focus;
		focus = (0 != (ev->active.state & SDL_APPACTIVE))
			? (focus | focusMinimized)
			: focus;
		
	}
	
	dest->focus.state = focus;
	return 0;

}

static int describe_focus_EV( ev_t* ev, int n, char* dest ) {

	char buf[256] = "Focus {";
	const char* bits[] = { " Mouse", " Keyboard", " Minimized" };

	int multi = 0;
	for( int i=0; i<sizeof(bits)/sizeof(bits[0]); i++ ) {

		if( 0 == (ev->focus.state & (1<<i)) )
			continue;

		if( multi )
			strcat( buf, ",");
		strcat( buf, bits[i] );
		multi = true;

	}
	strcat( buf, " }");
	return maybe_strncpy( dest, n, buf );

}

static int detail_focus_EV( ev_t* ev, int n, char* dest ) {

	return describe_focus_EV( ev, n, dest );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evFocus,
	.ev_size      = sizeof(ev_focus_t),
	.ev_mask      = sdl_ev_mask,

	.init_ev      = init_focus_EV,
	.translate_ev = translate_focus_EV,
	.describe_ev  = describe_focus_EV,
	.detail_ev    = detail_focus_EV

};
ev_adaptor_p       focus_EV_adaptor = &adaptor;
