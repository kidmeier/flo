#ifndef __in_joystick_h__
#define __in_joystick_h__

typedef struct joystick_s {

	int           index;

	const char*   name;
	int           n_axes;
	int           n_balls;
	int           n_buttons;
	int           n_hats;

} joystick_t;
typedef joystick_t* joystick_p;

int              joystick_count_IN(void);
const joystick_p joystick_info_IN( int n );

#endif
