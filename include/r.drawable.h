#ifndef __r_drawable_h__
#define __r_drawable_h__

#include "core.types.h"
#include "gl.array.h"
#include "gl.index.h"
#include "gl.shader.h"

typedef enum {

	drawNone   = -1,

	drawPoints,
	drawLines,
	drawLineStrip,
	drawLineLoop,
	drawTris,
	drawTriStrip,
	drawTriFan,
	drawQuads,

} drawMode_e;

typedef struct Drawable Drawable;
typedef struct Indexed_Drawable Indexed_Drawable;

struct Drawable {

	drawMode_e mode;

	Program*   pgm;

	Vindex*    els;
	Varray*    geo;

};

Drawable*     new_Drawable( Program* pgm, drawMode_e mode, Varray* geo );
Drawable*     new_Drawable_indexed( Program* pgm, 
                                    drawMode_e mode, 
                                    Vindex* els, 
                                    Varray* geo );
void       delete_Drawable( Drawable* dr );

#endif
