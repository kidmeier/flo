#include <assert.h>
#include <SDL/SDL_events.h>

#include "core.string.h"
#include "ev.mouse.h"
#include "time.core.h"

// WARNING: This is not re-entrant; should only be called from one thread.
int init_mouse_EV( ev_t* dest, const union SDL_Event* ev ) {

	static int16 X = 0, Y = 0;
	static uint16 buttons = 0;
		
	assert( 0 != (SDL_EVENTMASK(ev->type) & SDL_MOUSEEVENTMASK) );

	dest->mouse.prev_buttons = buttons;

	switch( ev->type ) {

	case SDL_MOUSEMOTION: {
		const SDL_MouseMotionEvent* motion = &ev->motion;

		dest->mouse.X.delta = (int16)X - motion->x;
		dest->mouse.Y.delta = (int16)Y - motion->y;
		dest->mouse.X.ord = motion->x;
		dest->mouse.Y.ord = motion->y;
		dest->mouse.buttons = buttons; //motion->state;

		X = motion->x; Y = motion->y; //buttons = motion->state;
		break;
	}
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP: {
		const SDL_MouseButtonEvent* button = &ev->button;

		dest->mouse.X.delta = (int16)X - button->x;
		dest->mouse.Y.delta = (int16)Y - button->y;
		dest->mouse.X.ord = button->x;
		dest->mouse.Y.ord = button->y;

		dest->mouse.buttons = (SDL_PRESSED == button->state)
			? (dest->mouse.buttons | (mouseButton(button->button)))
			: (dest->mouse.buttons & ~(mouseButton(button->button)));
		
		X = button->x; Y = button->y; buttons = dest->mouse.buttons;
		break;
	}
	default:
		return -1;
	}

	return 0;

}

int describe_mouse_EV( ev_t* ev, int n, char* dest ) {

	char buf[4092] = { '\0' };

	bool multi = false;
	uint16 buttons = ev->mouse.buttons; //mouseButtonsDelta(&ev->mouse);
	for( int i=0; i<mouseButtonsCount; i++ ) {

		if( !isMouseButtonDown(buttons,i) )
			continue;
		
		// Button NN
		char button[10];
		sprintf(button, "Button %d", i);

		if( multi )
			strcat(buf, " + ");
		strcat( buf, button );
		multi = true;

	}

	return maybe_strncpy( dest, n, buf );
}

int detail_mouse_EV( ev_t* ev, int n, char* dest ) {

	char buf[4092];
	char buttons[256];

	describe_mouse_EV( ev, 256, buttons );

	// (x+dx, y+dy) Mouse N <+ Mouse N <+ ...>>
	sprintf( buf, "(% 4d%+03d,% 4d%+03d) %s", 
	         ev->mouse.X.ord, ev->mouse.X.delta, 
	         ev->mouse.Y.ord, ev->mouse.Y.delta,
	         buttons );
	return maybe_strncpy( dest, n, buf );

}
