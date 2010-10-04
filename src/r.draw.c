#include <assert.h>
#include <stdarg.h>

#include "r.draw.h"

// Allocation
Draw*    alloc_Draw( zone_p Z ) {
	
	Draw* draw = zalloc( Z, sizeof(Draw) );
	draw->Z = Z;

	return draw;

}

Draw*     init_Draw( Draw* draw, Program* pgm ) {

	draw->pgm       = pgm;
	draw->n_attribs = attribc_Program(pgm);
	draw->attribs   = zalloc( draw->Z, draw->n_attribs * sizeof(draw->attribs[0]) );

	for( int i=0; i<draw->n_attribs; i++ ) {

		draw->attribs[i].attr  = i;
		draw->attribs[i].width = sizeof_Shader( attribi_Program(pgm, i)->type );
		
		draw->attribs[i].vbo   = NULL;
		draw->attribs[i].buf   = NULL;
		draw->attribs[i].wp    = NULL;
		draw->attribs[i].limit = NULL;

	}

	draw->mode = drawNone;

	return draw;

}

Draw*      new_Draw( zone_p Z, Program* pgm ) {

	return init_Draw( alloc_Draw(Z), pgm );

}

void      free_Draw( Draw* draw ) {

	zfree( draw->Z, draw );

}

void   destroy_Draw( Draw* draw ) {

	assert( drawNone == draw->mode );

	for( int i=0; i<draw->n_attribs; i++ )
		delete_Vattrib( draw->attribs[i].vbo );

	zfree( draw->Z, draw->attribs );

}

void    delete_Draw( Draw* draw ) {

	destroy_Draw( draw );
	free_Draw( draw );

}
	
// Mutators
Draw*    begin_Draw( Draw* draw, drawMode_e mode, uint count ) {

	assert( drawNone == draw->mode );

	// Allocate VBOs
	for( int i=0; i<draw->n_attribs; i++ ) {

		uint16 width = draw->attribs[i].width;

		draw->attribs[i].vbo   = new_Vattrib( attribi_Program(draw->pgm,i)->name,
		                                      width );
		draw->attribs[i].buf   = alloc_Vattrib( draw->attribs[i].vbo,
		                                        count * width,
		                                        staticDraw );
		
		draw->attribs[i].wp    = draw->attribs[i].buf;
		draw->attribs[i].limit = draw->attribs[i].buf + (count * width);

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

	for( int i=0; i<draw->n_attribs; i++ )
		attribs[i] = draw->attribs[i].vbo;

	Varray* varray = new_Varray( draw->n_attribs, attribs );
	Drawable* drawable = new_Drawable( draw->pgm, draw->mode, varray );

	return drawable;

}
