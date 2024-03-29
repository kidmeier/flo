#include <assert.h>
#include <SDL_events.h>

#include "core.log.h"
#include "core.string.h"
#include "ev.axis.h"
#include "in.joystick.h"

// Internal structs
struct axis_s {

	uint16 ord;
	int16  delta;

};

struct axis_set_s {

	const char* prefix;
	uint8 base;
	uint8 N;

	struct axis_s* axes;

};

// Module locals
static int                n_axis_sets = 0;
static struct axis_set_s*   axis_sets = NULL;

static uint8 init_axis_Ev( enable_ev_f enable, 
                           disable_ev_f disable, 
                           va_list args ) {
	
	if( NULL == axis_sets ) {
		
		int n_joysticks = joystick_count_IN();
		
		// Create the axis map; n_joysticks
		n_axis_sets = n_joysticks;
		axis_sets = calloc( n_axis_sets, sizeof(struct axis_set_s) );
		for( int i=0, base=0; i<n_axis_sets; i++ ) {
			
			struct joystick_s* joy = joystick_info_IN(i);
			
			axis_sets[i].prefix = joy->name;
			axis_sets[i].base   = base;
			axis_sets[i].N      = joy->n_axes;
			axis_sets[i].axes   = calloc( joy->n_axes, sizeof(struct axis_s) );
			memset( axis_sets[i].axes, 0, joy->n_axes * sizeof( struct axis_s ) );

			base += joy->n_axes;

		}
	}

	// Enable corresponding event type
	SDL_JoystickEventState( SDL_ENABLE );
	enable( SDL_JOYAXISMOTION );

	return 0;

}

// WARNING: This is not re-entrant; should only be called from one thread.
static int translate_axis_Ev( ev_t* dest, const union SDL_Event* ev ) {

	switch( ev->type ) {
		
	case SDL_JOYAXISMOTION: {
		
		const SDL_JoyAxisEvent* motion = &ev->jaxis;
		struct axis_set_s* axis_set = &axis_sets[ motion->which ];
		struct axis_s*         axis = &axis_set->axes[ motion->axis ];
		
		axis->delta = motion->value - axis->ord;
		axis->ord = motion->value;
		
		// Copy into dest evbuttonent
		dest->axis.which = axis_set->base + motion->axis;
		dest->axis.ord = axis->ord;
		dest->axis.delta = axis->delta;
		break;
		
	}
		
	default:
		fatal("Bad axis event: 0x%x", ev->type);
		return -1;
	}
	
	return 0;
	
}

static int describe_axis_Ev( const ev_t* ev, int n, char* dest ) {

	const int which = ev->axis.which;
	char buf[4092];

	// Find the axis set
	for( int i=0; i<n_axis_sets; i++ ) {
		
		struct axis_set_s* axis_set = &axis_sets[i];
		if( which >= axis_set->base 
		    && which < axis_set->base + axis_set->N ) {

			// Describe the axis event
			sprintf(buf, "%s axis %d", axis_set->prefix, which - axis_set->base);
			break;

		}

	}

	return maybe_strncpy( dest, n, buf );

}

static int detail_axis_Ev( const ev_t* ev, int n, char* dest ) {

	char buf[2048];
	char axis[2048];

	// Describe the axis and then write the detailed coord movement
	describe_axis_Ev( ev, sizeof(axis), axis );
	sprintf(buf, "%s: %d%+d", axis, ev->axis.ord, ev->axis.delta);

	return maybe_strncpy( dest, n, buf );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evAxis,
	.ev_size      = sizeof(ev_axis_t),

	.init_ev      = init_axis_Ev,
	.translate_ev = translate_axis_Ev,
	.describe_ev  = describe_axis_Ev,
	.detail_ev    = detail_axis_Ev

};
ev_adaptor_p       axis_Ev_adaptor = &adaptor;
