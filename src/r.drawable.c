#include "gl.array.h"
#include "gl.index.h"
#include "gl.shader.h"
#include "r.drawable.h"

Drawable* new_Drawable( region_p R, 
                        Program* pgm, 
                        drawMode_e mode, 
                        Varray* geo ) {

	return new_Drawable_indexed( R, pgm, mode, NULL, geo );

}

Drawable* new_Drawable_indexed( region_p R, 
                                Program* pgm, 
                                drawMode_e mode, 
                                Vindex* els, 
                                Varray* geo ) {

	Drawable* dr = ralloc( R, sizeof(Drawable) );
	dr->pgm = pgm;

	dr->mode = mode;
	dr->els = els;
	dr->geo = geo;

	return dr;

}

void      destroy_Drawable( Drawable* dr ) {

	delete_Program( dr->pgm );
	delete_Vindex( dr->els );
	delete_Varray( dr->geo );

}
