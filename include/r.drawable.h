#ifndef __r_drawable_h__
#define __r_drawable_h__

#include <GL/glew.h>

#include "core.types.h"
#include "gl.array.h"
#include "gl.index.h"
#include "gl.shader.h"
#include "mm.region.h"

typedef enum {

	drawNone      = -1,

	drawPoints    = GL_POINTS,
	drawLines     = GL_LINES,
	drawLineStrip = GL_LINE_STRIP,
	drawLineLoop  = GL_LINE_LOOP,
	drawTris      = GL_TRIANGLES,
	drawTriStrip  = GL_TRIANGLE_STRIP,
	drawTriFan    = GL_TRIANGLE_FAN,
	drawQuads     = GL_QUADS,
	drawQuadStrip = GL_QUAD_STRIP

} drawMode_e;

typedef struct Drawable Drawable;

struct Drawable {

	drawMode_e mode;
	
	uint       count;

	Vindex*    els;
	Varray*    geo;

};

Drawable*     new_Drawable( region_p, uint, Varray*, drawMode_e );
Drawable*     new_Drawable_indexed( region_p   R, 
                                    uint       count,
                                    Vindex*    els, 
                                    Varray*    geo,
                                    drawMode_e mode );
void      destroy_Drawable( Drawable* dr );

void         draw_Drawable( Drawable* dr, int argc, Shader_Arg* argv );

#endif
