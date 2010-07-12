#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include "display.core.h"

#ifndef __TEST__

int main(int argc, char* argv[]) {
	
	// Initialize SDL
	if( SDL_Init( SDL_INIT_NOPARACHUTE ) < 0 ) {
		
		fprintf(stderr, "Error: SDL_Init(): %s\n", SDL_GetError());
		exit(1);
		
	}
	
	if( set_DISPLAY( 512, 288, 0, 0,
	                 redBits, 8,
	                 greenBits, 8,
	                 blueBits, 8,
	                 alphaBits, 8,
	                 
	                 depthBits, 24,
	                 
	                 doubleBuffer, 1,
	                 vsync, 1,
	                 requireAccel, 1,
	                 -1) < 0 ) {
		fprintf(stderr, "Error: Failed to open display\n");
		exit(1);
	}
	
	SDL_Event ev;
	while( SDL_WaitEvent(&ev) >= 0 ) {
		
		switch(ev.type) {
			
		case SDL_QUIT: {
			printf("quitting\n");
			exit(0);
			break;
		}
			
		default:
			break;
		}
	}
	
}

#endif
