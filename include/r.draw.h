#ifndef __r_draw_h__
#define __r_draw_h__

#include "core.types.h"
#include "gl.attrib.h"
#include "gl.index.h"
#include "mm.region.h"
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

typedef struct Draw_Attrib Draw_Attrib;

struct Draw_Attrib {
	
	uint16     attr;
	uint16     width;
	
	Vattrib*   vbo;
	pointer    buf;
	pointer    wp;
	pointer    limit;
	
};

typedef struct Draw Draw;

struct Draw {

	region_p       R;

	Program*       pgm;
	uint         n_attribs;
	Draw_Attrib*   attribs;

	drawMode_e     mode;

};

// Allocation
Draw*    alloc_Draw( region_p );
Draw*     init_Draw( Draw*, Program* pgm );
Draw*      new_Draw( region_p, Program* pgm );

void      free_Draw( Draw* );
void   destroy_Draw( Draw* );
void    delete_Draw( Draw* );

// Mutators
Draw*    begin_Draw( Draw*, drawMode_e mode, uint count );
Draw*   vertex_Draw( Draw*, ... );
Drawable*  end_Draw( Draw* );

#endif
