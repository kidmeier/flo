#include <SDL_mouse.h>
#include "in.mouse.h"

bool      is_Mouse_captured( void ) {

	return SDL_TRUE == SDL_GetRelativeMouseMode();

}

int      set_Mouse_captured( bool captured ) {

	return SDL_SetRelativeMouseMode( captured ? SDL_TRUE : SDL_FALSE );

}
