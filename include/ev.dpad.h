#ifndef __ev_dpad_h__
#define __ev_dpad_h__

#include "ev.core.h"

#define dpadCentered 0x00
#define dpadUp       0x01
#define dpadRight    0x02
#define dpadDown     0x04
#define dpadLeft     0x08

#define dpadRightUp   (dpadRight | dpadUp)
#define dpadRightDown (dpadRight | dpadDown)
#define dpadLeftUp    (dpadLeft | dpadUp)
#define dpadLeftDown  (dpadLeft | dpadDown)

extern ev_adaptor_p dpad_EV_adaptor;

#endif
