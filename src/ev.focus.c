#include <assert.h>
#include <SDL_events.h>

#include "core.log.h"
#include "core.string.h"
#include "ev.focus.h"

static uint8 init_focus_EV( enable_ev_f enable,
                            disable_ev_f disable,
                            va_list args ) {

	enable( SDL_WINDOWEVENT );
	
	return 0;

}

// WARNING: This is not re-entrant; should only be called from one thread.
static int translate_focus_EV( ev_t* dest, const union SDL_Event* ev ) {

	static uint8 focus = 0;


	assert( SDL_WINDOWEVENT == ev->type );

	switch( ev->window.event ) {

	case SDL_WINDOWEVENT_ENTER:
		focus |= focusMouse;
		break;

	case SDL_WINDOWEVENT_LEAVE:
		focus &= ~(focusMouse);
		break;

	case SDL_WINDOWEVENT_FOCUS_GAINED:
		focus |= focusKeyboard;
		break;

	case SDL_WINDOWEVENT_FOCUS_LOST:
		focus &= ~(focusKeyboard);
		break;

	default:
		fatal("Bad focus event: 0x%x", ev->window.event);
		return -1;
	}

	dest->focus.state = focus;
	return 0;

}

static int describe_focus_EV( const ev_t* ev, int n, char* dest ) {

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

static int detail_focus_EV( const ev_t* ev, int n, char* dest ) {

	return describe_focus_EV( ev, n, dest );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evFocus,
	.ev_size      = sizeof(ev_focus_t),

	.init_ev      = init_focus_EV,
	.translate_ev = translate_focus_EV,
	.describe_ev  = describe_focus_EV,
	.detail_ev    = detail_focus_EV

};
ev_adaptor_p       focus_EV_adaptor = &adaptor;
