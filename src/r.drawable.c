#include "gl.array.h"
#include "gl.index.h"
#include "gl.shader.h"
#include "r.drawable.h"

Drawable* new_Drawable( region_p R, 
                        uint    count,
                        Varray* geo,
                        drawMode_e mode ) {

	return new_Drawable_indexed( R, count, NULL, geo, mode );

}

Drawable* new_Drawable_indexed( region_p   R, 
                                uint       count,
                                Vindex*    els, 
                                Varray*    geo,
                                drawMode_e mode ) {

	Drawable* dr = ralloc( R, sizeof(Drawable) );

	dr->mode  = mode;
	dr->count = count;
	dr->els = els;
	dr->geo = geo;

	return dr;

}

void      destroy_Drawable( Drawable* dr ) {

	if( dr->els )
		delete_Vindex( dr->els );
	delete_Varray( dr->geo );

}

void         draw_Drawable( Drawable* dr, Shader_Arg* argv ) {

	load_Program_uniforms( argv );

	if( dr->els )
		draw_Varray_indexed( dr->els, dr->geo, dr->mode, 0, dr->count );
	else
		draw_Varray( dr->geo, dr->mode, 0, dr->count  );

}
