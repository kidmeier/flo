#include <assert.h>
#include <SDL_events.h>

#include "core.log.h"
#include "core.string.h"
#include "ev.cursor.h"
#include "in.joystick.h"

struct cursor_s {

	uint16 X,  Y;
	int16 dX, dY;

};

struct cursor_set_s {

	const char* prefix;

	uint8 base;
	uint8 N;

	struct cursor_s* cursors;

};

// Module locals
static struct cursor_s     mouse_cursor = { 0, 0, 0, 0 };
static struct cursor_set_s        mouse = { "Mouse", 0, 1, &mouse_cursor };

static int                n_cursor_sets = 0;
static struct cursor_set_s* cursor_sets = NULL;

static uint8 init_cursor_EV( enable_ev_f enable, 
                             disable_ev_f disable,
                             va_list args ) {

	if( NULL == cursor_sets ) {
		
		int n_joysticks = joystick_count_IN();
		
		// Create the axis map; 1 (mouse) + n_joysticks
		n_cursor_sets = 1 + n_joysticks;
		cursor_sets = calloc( n_cursor_sets, sizeof(struct cursor_set_s) );

		// Assign mouse
		cursor_sets[0] = mouse;

		// Assign joysticks
		for( int i=1, base=1; i<n_cursor_sets; i++ ) {
			
			struct joystick_s* joy = joystick_info_IN(i-1);
			
			cursor_sets[i].prefix  = joy->name;
			cursor_sets[i].base    = base;
			cursor_sets[i].N       = joy->n_balls;
			cursor_sets[i].cursors = calloc( joy->n_balls, sizeof(struct cursor_s) );
			memset( cursor_sets[i].cursors, 0, joy->n_balls * sizeof( struct cursor_s ) );

			base += joy->n_balls;

		}
	}

	enable( SDL_MOUSEMOTION );
	enable( SDL_JOYBALLMOTION );
	
	return 0;

}

// WARNING: This is not re-entrant; should only be called from one thread.
static int translate_cursor_EV( ev_t* dest, const union SDL_Event* ev ) {

	struct cursor_s* cursor = NULL;

	// Figure out which cursor the event maps to and update it
	switch( ev->type ) {

	case SDL_MOUSEMOTION: {
		const SDL_MouseMotionEvent* motion = &ev->motion;
		struct cursor_set_s* cursor_set = &cursor_sets[0];

		cursor = &cursor_set->cursors[0];
		dest->cursor.which = cursor_set->base;

		cursor->dX = (int16)motion->x - cursor->X;
		cursor->dY = (int16)motion->y - cursor->Y;
		cursor->X = motion->x;
		cursor->Y = motion->y;	
		break;

	}
	case SDL_JOYBALLMOTION: {
		const SDL_JoyBallEvent* motion = &ev->jball;
		struct cursor_set_s* cursor_set = &cursor_sets[ 1 + motion->which ];
		
		cursor = &cursor_set->cursors[ motion->ball ];
		dest->cursor.which = cursor_set->base + motion->ball;

		// TODO: Clamping!?!?
		cursor->dX = motion->xrel;
		cursor->dY = motion->yrel;
		cursor->X += motion->xrel;
		cursor->Y += motion->yrel;
		break;

	}
	default:
		fatal("Bad cursor event: 0x%x", ev->type);
		return -1;
	}

	// Copy into dest
	dest->cursor.X = cursor->X;
	dest->cursor.Y = cursor->Y;
	dest->cursor.dX = cursor->dX;
	dest->cursor.dY = cursor->dY;
	
	return 0;
		
}

static int describe_cursor_EV( const ev_t* ev, int n, char* dest ) {

	const int which = ev->axis.which;
	char buf[4092];

	// Find the axis set
	for( int i=0; i<n_cursor_sets; i++ ) {
		
		struct cursor_set_s* cursor_set = &cursor_sets[i];
		if( which >= cursor_set->base 
		    && which < cursor_set->base + cursor_set->N ) {

			// Describe the axis event
			sprintf(buf, "%s cursor %d", cursor_set->prefix, which - cursor_set->base);
			break;

		}

	}

	return maybe_strncpy( dest, n, buf );

}

static int detail_cursor_EV( const ev_t* ev, int n, char* dest ) {

	char buf[2048];
	char cursor[2048];

	// Describe the cursor and then write the detailed coord movement
	describe_cursor_EV( ev, sizeof(cursor), cursor );
	sprintf(buf, "%s: (%d%+d, %d%+d)", cursor, 
	        ev->cursor.X, ev->cursor.dX,
	        ev->cursor.Y, ev->cursor.dY);

	return maybe_strncpy( dest, n, buf );

}

// Export the event adaptor
static ev_adaptor_t adaptor = {

	.ev_type      = evCursor,
	.ev_size      = sizeof(ev_cursor_t),

	.init_ev      = init_cursor_EV,
	.translate_ev = translate_cursor_EV,
	.describe_ev  = describe_cursor_EV,
	.detail_ev    = detail_cursor_EV

};
ev_adaptor_p       cursor_EV_adaptor = &adaptor;
