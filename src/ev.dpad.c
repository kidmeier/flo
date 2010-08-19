#include <assert.h>
#include <SDL/SDL_events.h>

#include "core.alloc.h"
#include "core.string.h"
#include "ev.dpad.h"
#include "in.joystick.h"

// Internal structs
struct dpad_set_s {

	const char* prefix;
	uint8       base;
	uint8       N;

	uint8*      dpads;

};

// Module locals
static pointer                   pool = NULL;

static int                n_dpad_sets = 0;
static struct dpad_set_s*   dpad_sets = NULL;

#define sdl_ev_mask SDL_JOYHATMOTIONMASK

static uint32 init_dpad_EV( va_list args ) {

	if( NULL == pool ) {
		
		pool = autofree_pool();
		
		int n_joysticks = joystick_count_IN();
		
		// Create the axis map; n_joysticks
		n_dpad_sets = n_joysticks;
		dpad_sets = new_array( pool, struct dpad_set_s, n_dpad_sets );
		for( int i=0, base=0; i<n_dpad_sets; i++ ) {
			
			struct joystick_s* joy = joystick_info_IN(i);
			
			dpad_sets[i].prefix = joy->name;
			dpad_sets[i].base   = base;
			dpad_sets[i].N      = joy->n_hats;
			dpad_sets[i].dpads  = new_array( pool, uint8, joy->n_hats );
			memset( dpad_sets[i].dpads, 0, joy->n_hats * sizeof( uint8 ) );

			base += joy->n_hats;

		}
	}

	// All buttons
	return sdl_ev_mask;

}

// WARNING: This is not re-entrant; should only be called from one thread.
static int translate_dpad_EV( ev_t* dest, const union SDL_Event* ev ) {

	assert( 0 != (SDL_EVENTMASK(ev->type) & sdl_ev_mask) );
	
	switch( ev->type ) {

	case SDL_JOYHATMOTION: {
		
		const SDL_JoyHatEvent* jhat = &ev->jhat;
		struct dpad_set_s* dpad_set = &dpad_sets[ jhat->which ];
		uint8*                  hat = &dpad_set->dpads[ jhat->hat ];
		
		*hat = jhat->value;

		// Copy into dest event
		dest->dpad.which = dpad_set->base + jhat->hat;
		dest->dpad.pos = *hat;
		break;
		
	}

	default:
		return -1;
	}

	return 0;

}

static int describe_dpad_EV( ev_t* ev, int n, char* dest ) {

	const int which = ev->dpad.which;
	char buf[4092];

	// Find the axis set
	for( int i=0; i<n_dpad_sets; i++ ) {
		
		struct dpad_set_s* dpad_set = &dpad_sets[i];
		if( which >= dpad_set->base 
		    && which < dpad_set->base + dpad_set->N ) {

			// Describe the axis event
			sprintf(buf, "%s dpad %d", dpad_set->prefix, which - dpad_set->base);
			break;

		}

	}

	return maybe_strncpy( dest, n, buf );

}

static int detail_dpad_EV( ev_t* ev, int n, char* dest ) {

	static const char* dir_map[] = {
		[dpadCentered] = "Centered",
		[dpadUp] = "Up",
		[dpadRightUp] = "Right up",
		[dpadRight] = "Right",
		[dpadRightDown] = "Right down",
		[dpadDown] = "Down",
		[dpadLeftDown] = "Down left",
		[dpadLeft] = "Left",
		[dpadLeftUp] = "Left up"
	};
	char buf[2048];
	char dpad[2048];
	
	// Describe the hat and its position
	describe_dpad_EV( ev, sizeof(dpad), dpad );
	sprintf(buf, "%s: %s", dpad, dir_map[ ev->dpad.pos] );
	
	return maybe_strncpy( dest, n, buf );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evDpad,
	.ev_size      = sizeof(ev_dpad_t),
	.ev_mask      = sdl_ev_mask,

	.init_ev      = init_dpad_EV,
	.translate_ev = translate_dpad_EV,
	.describe_ev  = describe_dpad_EV,
	.detail_ev    = detail_dpad_EV

};
ev_adaptor_p       dpad_EV_adaptor = &adaptor;