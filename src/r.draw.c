#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "r.draw.h"

// Allocation
Draw*    alloc_Draw( region_p R ) {
	
	Draw* draw = ralloc( R, sizeof(Draw) );
	draw->R    = R;

	return draw;

}

Draw*     init_Draw( Draw* draw, Program* pgm ) {

	draw->pgm       = pgm;
	draw->n_attribs = attribc_Program(pgm);
	draw->attribs   = ralloc( draw->R, draw->n_attribs * sizeof(Draw_Attrib) );

	for( int i=0; i<draw->n_attribs; i++ ) {

		Draw_Attrib* attrib = &draw->attribs[i];
		
		attrib->attr  = i;
		attrib->width = sizeof_Shader( attribi_Program(pgm, i)->type );
		
		attrib->vbo   = NULL;
		attrib->buf   = NULL;
		attrib->wp    = NULL;
		attrib->limit = NULL;

	}

	draw->mode = drawNone;

	return draw;

}

Draw*      new_Draw( region_p R, Program* pgm ) {

	return init_Draw( alloc_Draw(R), pgm );

}

void   destroy_Draw( Draw* draw ) {

	assert( drawNone == draw->mode );

	for( int i=0; i<draw->n_attribs; i++ )
		delete_Vattrib( draw->attribs[i].vbo );

}

// Mutators
Draw*    begin_Draw( Draw* draw, drawMode_e mode, uint count ) {

	assert( drawNone == draw->mode );

	// Allocate VBOs
	for( int i=0; i<draw->n_attribs; i++ ) {

		Draw_Attrib* attrib = &draw->attribs[i];
		uint16        width = attrib ->width;

		attrib->vbo   = new_Vattrib( attribi_Program(draw->pgm,i)->name,
		                             width );
		attrib->buf   = alloc_Vattrib( attrib->vbo,
		                               count * width,
		                               staticDraw );
		
		attrib->wp    = attrib->buf;
		attrib->limit = attrib->buf + (count * width);
		
	}

	// Set mode
	draw->mode = mode;

	return draw;

}

Draw*   vertex_Draw( Draw* draw, ... ) {

	va_list argv;

	// We must be between begin_Draw .. end_Draw
	assert( drawNone != draw->mode );

#ifdef _DEBUG

	// Check bounds
	for( int i=0; i<draw->n_attribs; i++ ) {

		if( draw->attribs[i].wp + draw->attribs[i].width > draw->attribs[i].limit )
			return NULL;

	}

#endif

	// Write the attribs
	va_start( argv, draw );
	for( int i=0; i<draw->n_attribs; i++ ) {

		pointer attrv = va_arg( argv, pointer );
		memcpy( draw->attribs[i].wp, attrv, draw->attribs[i].width );

		draw->attribs[i].wp += draw->attribs[i].width;

	}
	va_end( argv );

	return draw;

}

Drawable*  end_Draw( Draw* draw ) {

	Vattrib* attribs[ draw->n_attribs ];

	for( int i=0; i<draw->n_attribs; i++ ) {

		flush_Vattrib( draw->attribs[i].vbo );
		attribs[i] = draw->attribs[i].vbo;

	}

	Varray*   varray = new_Varray( draw->n_attribs, attribs );
	Drawable*  drwbl = new_Drawable( draw->R, draw->pgm, draw->mode, varray );

	return drwbl;

}
