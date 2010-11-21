#include <assert.h>
#include <SDL_events.h>

#include "core.alloc.h"
#include "core.log.h"
#include "core.string.h"
#include "data.bitset.h"
#include "ev.button.h"

#include "in.joystick.h"

// A button set is an abstract range of button numbers which represent 
// buttons on some device
struct buttonset_s {

	const char* prefix;
	uint8 base;
	uint8 N;

};

// Module locals
static pointer     pool = NULL;

// SDL doesn't have a way to figure out the number of buttons on the actual 
// mouse (nor the number of mice); we will assume 16 is plenty (hopefully)
static struct buttonset_s mouseset = { "Mouse", 0, 16 };

static int                 n_buttonsets;
static struct buttonset_s* buttonsets = NULL;
static        bitset( button_state, maxButtonCount );

static uint8 init_button_EV( enable_ev_f enable,
                             disable_ev_f disable,
                             va_list args ) {

	if( NULL == pool ) {

		pool = autofree_pool();

		// Clean slate
		bitset_clearall( button_state, maxButtonCount );

		// Create the button map; 1 (mouse) + n_joysticks
		int n_joysticks = joystick_count_IN();
		n_buttonsets = 1 + n_joysticks;
		buttonsets = new_array( pool, struct buttonset_s, n_buttonsets );
		buttonsets[0] = mouseset;
		for( int i=1, base=mouseset.N; i<n_buttonsets; i++ ) {

			struct joystick_s* joy = joystick_info_IN(i-1);

			buttonsets[i].prefix = joy->name;
			buttonsets[i].base   = base;
			buttonsets[i].N      = joy->n_buttons;

			base += joy->n_buttons;

		}
	}

	enable( SDL_MOUSEBUTTONDOWN );
	enable( SDL_MOUSEBUTTONUP );
	enable( SDL_JOYBUTTONDOWN );
	enable( SDL_JOYBUTTONUP );

	return 0;

}

// WARNING: This is not re-entrant; should only be called from one thread.
static int translate_button_EV( ev_t* dest, const union SDL_Event* ev ) {

	int  which;
	bool pressed;
	switch( ev->type ) {

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP: {
		const SDL_MouseButtonEvent* button = &ev->button;

		which = buttonsets[0].base + ev->button.button;
		pressed = SDL_PRESSED == button->state;
		break;
	}

	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP: {
		const SDL_JoyButtonEvent* button = &ev->jbutton;
		struct buttonset_s* buttonset = &buttonsets[ 1+button->which ];
		
		which = buttonset->base + button->button;
		pressed = SDL_PRESSED == button->state;
		break;
	}
	default:
		fatal("Bad button event: 0x%x", ev->type);
		return -1;
	}

	// Update our local state
	if( pressed )
		bitset_set( button_state, which );
	else
		bitset_clear( button_state, which );

	// Copy into dest event
	dest->button.which = which;
	dest->button.pressed = pressed;
	bitset_copy( dest->button.state, maxButtonCount, button_state );

	return 0;

}

static int describe_button_EV( ev_t* ev, int n, char* dest ) {

	const int which = ev->button.which;
	char buf[4092];

	// Find the button set
	for( int i=0; i<n_buttonsets; i++ ) {
		
		struct buttonset_s* buttonset = &buttonsets[i];
		if( which >= buttonset->base 
		    && which < buttonset->base + buttonset->N ) {

			// Describe the button press/release
			sprintf(buf, "%s %s button %d", 
			        bitset_isset( ev->button.state, which ) ? "Pressed" : "Released",
			        buttonset->prefix, which - buttonset->base);
			break;

		}

	}

	return maybe_strncpy( dest, n, buf );

}

static int detail_button_EV( ev_t* ev, int n, char* dest ) {

	char buf[4092] = { '\0' };

	// Describe the button press
	describe_button_EV( ev, sizeof(buf), buf );
	strcat( buf, " State { " );

	// Write the rest of the button states
	bool multi = false;
	for( int i=0; i<n_buttonsets; i++ ) {

		struct buttonset_s* buttonset = &buttonsets[i];

		for( int j=0; j<buttonset->N; j++ ) {
			int button = buttonset->base + j;

			if( bitset_isset( ev->button.state, button ) ) {
				// Button NN
				char desc[256];
				sprintf(desc, "%s button %d", buttonset->prefix, j);

				if( multi )
					strcat(buf, ", ");
				strcat( buf, desc );
				multi = true;
			}
		}
	}

	strcat( buf, " }");
	return maybe_strncpy( dest, n, buf );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evButton,
	.ev_size      = sizeof(ev_button_t),

	.init_ev      = init_button_EV,
	.translate_ev = translate_button_EV,
	.describe_ev  = describe_button_EV,
	.detail_ev    = detail_button_EV

};
ev_adaptor_p       button_EV_adaptor = &adaptor;
