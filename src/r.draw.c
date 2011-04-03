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

		Shader_Param* shParm = attribi_Program(pgm, i);
		Draw_Attrib* attrib = &draw->attribs[ shParm->loc ];
		Shader_Type* type   = &shParm->type;

		attrib->type   = type->gl_type;
		attrib->prim   = type->prim;
		attrib->size   = type->shape[1];
		attrib->stride = type->primSize * type->shape[0] * type->shape[1] * type->length;
		
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

		Shader_Param* shParm = attribi_Program(draw->pgm, i);
		Draw_Attrib* attrib  = &draw->attribs[ shParm->loc ];

		attrib->vbo   =  new_Vattrib( shParm->name,
		                              attrib->size, attrib->prim, GL_FALSE );
		attrib->buf   = alloc_Vattrib( attrib->vbo,
		                               staticDraw,
		                               count );
		
		attrib->wp    = attrib->buf;
		attrib->limit = attrib->buf + (count * attrib->stride);
		
	}

	// Set mode
	draw->mode  = mode;
	draw->count = count;

	return draw;

}

Draw*   vertex_Draw( Draw* draw, ... ) {

	// We must be between begin_Draw .. end_Draw
	assert( drawNone != draw->mode );

#ifdef DEBUG

	// Check bounds
	for( int i=0; i<draw->n_attribs; i++ ) {

		Draw_Attrib* attrib = &draw->attribs[i];
		if( attrib->wp + attrib->stride > attrib->limit )
			return NULL;

	}

#endif

	// Write the attribs
	va_list argv;

	va_start( argv, draw );
	for( int i=0; i<draw->n_attribs; i++ ) {

		pointer attrv = va_arg( argv, pointer );

		memcpy( draw->attribs[i].wp, attrv, draw->attribs[i].stride );
		draw->attribs[i].wp += draw->attribs[i].stride;

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
	Drawable*  drwbl = new_Drawable( draw->R, 
	                                 draw->count,
	                                 varray,
	                                 draw->mode );

	return drwbl;

}

#ifdef __r_draw_TEST__

#include <stdlib.h>
#include <stdio.h>

#include <SDL.h>

#include "core.log.h"

#include "gl.context.h"
#include "gl.display.h"
#include "gl.shader.h"

#include "sync.thread.h"

const char* vertex_glsl = "\
                           \
uniform float coldest;     \
uniform float range;       \
                           \
attribute float vertexTemperature;	\
attribute vec2  vertex;	\
                           \
varying float temperature; \
                           \
void main() {	\
	\
  temperature = (vertexTemperature - coldest) / range;	\
  gl_Position = gl_ProjectionMatrix * vec4(vertex, 0.0, 1.0); \
	\
}	\
";

const char* fragment_glsl = "\
                             \
uniform vec3 cold;           \
uniform vec3 hot;            \
                             \
varying float temperature;   \
                             \
void main() {                \
                             \
  vec3 color = mix( cold, hot, temperature );	\
  gl_FragColor = vec4(color*color, 1.0);	\
	\
}	\
";

Drawable* drawCircle( region_p R, Program* proc, int vertices, float radius ) {

	assert( vertices >= 4 );

	Draw *circle = new_Draw( R, proc );

	begin_Draw( circle, drawTriFan, 1 + vertices + 1);

	float t0    = 0.f;
	float pt0[2] = { 0.f, 0.f };

	vertex_Draw( circle, &pt0[0], &t0 );
	for( int i=0; i<vertices; i++ ) {

		float arc = (2.f * M_PI) * (float)i / (float)vertices;
		float pt[2];

		pt[0] = radius * cosf(arc);
		pt[1] = radius * sinf(arc);

		float t = 0.5f * (1.f + sinf( M_PI * arc ));
		vertex_Draw( circle, &pt[0], &t );

	}
	// Close it
	float ptN[2] = { radius, 0.f };
	float tN = 0.5f;
	vertex_Draw( circle, &ptN[0], &tN );

	return end_Draw( circle );

}

int main( int argc, char* argv[] ) {

	if( SDL_Init( SDL_INIT_EVERYTHING | SDL_INIT_NOPARACHUTE ) < 0 )
		fatal0( "SDL_Init failed" );
	
	const int width  = 512;
	const int height = 288;
	const float aspect = (float)width / (float)height;
	Display* display = open_Display( "Flo",
	                                 width, height, 0,
	                                 
	                                 redBits, 8,
	                                 greenBits, 8,
	                                 blueBits, 8,
//	                                 alphaBits, 8,
	                                 
	                                 depthBits, 24,
	                                 
	                                 doubleBuffer, 1,
	                                 requireAccel, 1,
	                                 
	                                 glMajor, 3,
	                                 glMinor, 2,
	                                 
	                                 -1 );
	if( !display )
		fatal0( "Failed to open display");

	Glcontext gl = create_Glcontext( display );
	if( !gl )
		fatal0( "Failed to create GL context" );

	bind_Glcontext( display, gl );

	// Prepare OpenGL state
	glViewport( 0, 0, width, height );
	glClearColor( 0.f, 0.f, 0.f, 1.f );
	glClear( GL_COLOR_BUFFER_BIT );
	glMatrixMode( GL_PROJECTION );
	glOrtho( -1.0*aspect, 1.0*aspect, -1.0, 1.0, -1.0, 1.0 );

	region_p R = region("r.draw.test");
	Shader   *vertexSh   = compile_Shader( shadeVertex, 
	                                     "temperatureVertex", 
	                                     vertex_glsl );
	Shader   *fragmentSh = compile_Shader( shadeFragment, 
	                                       "temperatureFragment", 
	                                       fragment_glsl );
	Program  *proc       = define_Program( "temperatureProgram",
	                                       2, vertexSh, fragmentSh,
	                                       2, "vertex", "vertexTemperature",
	                                       4, "coldest", "range", "cold", "hot"
		);

	Drawable* circle  = drawCircle( R, proc, 32, 0.95f );

	// uniforms
	float     coldest = 0.f;
	float     range   = 1.f;
	float     cold[3] = { 0.145f, 0.129f, 0.3529f };
	float     hot [3] = { 1.f, 0.647f, 0.f };

	int         unic = uniformc_Program( proc );
	Shader_Arg* univ = bind_Shader_argv( R, 
	                                     unic,
	                                     uniformv_Program( proc ),
	                                     &coldest, &range, cold, hot );

	use_Program( proc, NULL );
	draw_Drawable( circle, unic, univ );

	flip_Display( display );

	getc( stdin );

	destroy_Drawable( circle );
	delete_Glcontext( gl );
	close_Display( display );

}

#endif
