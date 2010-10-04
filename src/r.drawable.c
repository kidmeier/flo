#include "core.alloc.h"
#include "gl.array.h"
#include "gl.index.h"
#include "gl.shader.h"
#include "r.drawable.h"

Drawable* new_Drawable( Program* pgm, drawMode_e mode, Varray* geo ) {

	return new_Drawable_indexed( pgm, mode, NULL, geo );

}

Drawable* new_Drawable_indexed( Program* pgm, drawMode_e mode, Vindex* els, Varray* geo ) {

	Drawable* dr = new( NULL, Drawable );
	dr->pgm = pgm;

	dr->mode = mode;
	dr->els = els;
	dr->geo = geo;

	return dr;

}

void       delete_Drawable( Drawable* dr ) {

	delete_Program( dr->pgm );
	delete_Vindex( dr->els );
	delete_Varray( dr->geo );

	delete( dr );

}
