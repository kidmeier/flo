#ifndef __ev_button_h__
#define __ev_button_h__

#include "data.bitset.h"
#include "ev.core.h"

#define isButtonPressed(evp,which)	  \
	bitset_isset( (evp)->button.state, which )

extern ev_adaptor_p button_Ev_adaptor;

#endif
