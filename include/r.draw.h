#ifndef __r_draw_h__
#define __r_draw_h__

#include "core.types.h"
#include "gl.attrib.h"
#include "gl.index.h"
#include "mm.zone.h"
#include "r.drawable.h"

typedef enum {

	attrNone      = -1,

	attrPosition,
	attrColor,
	attr2ndColor,
	attrNormal,
	attrTexCoord,

	attrCustom0

} drawAttrib_e;

typedef struct Draw Draw;

struct Draw {

	zone_p         Z;

	Program*       pgm;
	uint         n_attribs;
	struct {

		uint16     attr;
		uint16     width;

		Vattrib*   vbo;
		pointer    buf;
		pointer    wp;
		pointer    limit;

	}*             attribs;

	drawMode_e     mode;

};

// Allocation
Draw*    alloc_Draw( zone_p );
Draw*     init_Draw( Draw*, Program* pgm );
Draw*      new_Draw( zone_p Z, Program* pgm );

void      free_Draw( Draw* );
void   destroy_Draw( Draw* );
void    delete_Draw( Draw* );

// Mutators
Draw*    begin_Draw( Draw*, drawMode_e mode, uint count );
Draw*   vertex_Draw( Draw*, ... );
Drawable*  end_Draw( Draw* );

#endif
