#ifndef __r_drawable_h__
#define __r_drawable_h__

#include "core.types.h"
#include "gl.array.h"
#include "gl.index.h"
#include "gl.shader.h"
#include "mm.region.h"
#include "r.xform.h"

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

struct Drawable {

	drawMode_e mode;

	Program*   pgm;

	Vindex*    els;
	Varray*    geo;

};

Drawable*     new_Drawable( region_p, Program*, drawMode_e, Varray* );
Drawable*     new_Drawable_indexed( region_p, 
                                    Program*, 
                                    drawMode_e, 
                                    Vindex*, 
                                    Varray* );
void      destroy_Drawable( Drawable* dr );

#endif
