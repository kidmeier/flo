#include <assert.h>
#include <SDL/SDL_events.h>

#include "core.string.h"
#include "ev.focus.h"

// WARNING: This is not re-entrant; should only be called from one thread.
int init_focus_EV( ev_t* dest, const union SDL_Event* ev ) {

	static uint8 focus = 0;

	assert( SDL_ACTIVEEVENT == ev->type );

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

int describe_focus_EV( ev_t* ev, int n, char* dest ) {

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

int detail_focus_EV( ev_t* ev, int n, char* dest ) {

	return describe_focus_EV( ev, n, dest );

}
