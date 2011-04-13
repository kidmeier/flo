#include <assert.h>

#include <stdarg.h>
#include <stdlib.h>

#include "core.log.h"
#include "r.target.h"

static Framebuffer *current_fb = NULL;

Rendertarget * create_Renderbuffer( framebufferAttachment_e attachment,
                                    GLenum format ) {
	GLuint id; glGenRenderbuffers( 1, &id );

	if( !id )
		return NULL;

	Rendertarget *targ = malloc( sizeof(Rendertarget) );
	if( !targ ) {
		glDeleteRenderbuffers( 1, &id );
		return NULL;
	}
	
	targ->id         = id;
	targ->type       = GL_RENDERBUFFER;
	targ->attachment = attachment;
	targ->format     = format;

	return targ;
	
}

Rendertarget * create_Rendertexture( GLenum target,
                                     framebufferAttachment_e attachment,
                                     GLenum format ) {

	GLuint id; glGenTextures( 1, &id );

	if( !id )
		return NULL;

	Rendertarget *targ = malloc( sizeof(Rendertarget) );
	if( !targ ) {
		glDeleteTextures( 1, &id );
		return NULL;
	}
	
	targ->id         = id;
	targ->type       = target;
	targ->attachment = attachment;
	targ->format     = format;

	return targ;

}

void          destroy_Rendertarget( Rendertarget* targ ) {

	if( GL_RENDERBUFFER == targ->type )
		glDeleteRenderbuffers( 1, &targ->id );
	else
		glDeleteTextures( 1, &targ->id );

	free( targ );

}

Framebuffer  * define_Framebuffer( GLint width, GLint height, 
                                   int count, ... ) {

	Rendertarget *targets[ count ];
	va_list argv;

	va_start( argv, count );
	for( int i=0; i<count; i++ )
		targets[i] = va_arg( argv, Rendertarget* );
	va_end( argv );

	return new_Framebuffer( width, height, count, targets );

}

Framebuffer  *    new_Framebuffer( GLint width, GLint height, 
                                   int count, Rendertarget *targets[] ) {

	assert( count > 0 );

	GLuint id; glGenFramebuffers( 1, &id );

	if( !id )
		return NULL;

	Framebuffer *fb = malloc( sizeof(Framebuffer) 
	                          + count * sizeof(Rendertarget*) );
	if( !fb ) {
		glDeleteFramebuffers( 1, &id );
		return NULL;
	}

	fb->id = id;
	fb->width = width;
	fb->height = height;
	fb->targetc = count;

	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fb->id );
	for( int i=0; i<fb->targetc; i++ ) {

		Rendertarget *targ = targets[i];
		fb->targetv[i]     = targets[i];

		assert( NULL != targ );

		switch( targ->type ) {

		case GL_RENDERBUFFER:
			
			glBindRenderbuffer( GL_RENDERBUFFER, targ->id );
			glRenderbufferStorage( GL_RENDERBUFFER, targ->format, 
			                       width, height );
			glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, targ->attachment,
			                           GL_RENDERBUFFER, targ->id );
			break;

		case texture_2d:
		case texture_cubePositiveX:
		case texture_cubePositiveY:
		case texture_cubePositiveZ:
		case texture_cubeNegativeX:
		case texture_cubeNegativeY:
		case texture_cubeNegativeZ:
			
			glBindTexture( targ->type, targ->id );
			glTexParameteri( GL_TEXTURE_2D, 
			                 GL_TEXTURE_MIN_FILTER, 
			                 GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, 
			                 GL_TEXTURE_MAG_FILTER, 
			                 GL_LINEAR );
			glTexImage2D( targ->type, 0, targ->format, width, height, 0,
			              targ->format,
			              GL_UNSIGNED_INT,
			              NULL );
			glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, targ->attachment,
			                        targ->type, targ->id, 0 );
			break;

		case texture_1d:
			fatal0("unsupported render target: GL_TEXTURE_1D");
			break;

		case texture_3d:
			fatal0("unsupported render target: GL_TEXTURE_3D");
			break;

		default:
			fatal("unknown render target type: 0x%x", targ->type );
			break;

		}

	}

	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );

	return fb;

}
  
void          destroy_Framebuffer( Framebuffer *fb, bool targets ) {

	assert( NULL != fb );

	if( targets )
		for( int i=0; i<fb->targetc; i++ )
			destroy_Rendertarget( fb->targetv[i] );

	free( fb );

}

bool          validate_Framebuffer( void ) {

	return GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

}

Framebuffer  *    bind_Framebuffer( Framebuffer *fb ) {

	Framebuffer *last = current_fb;

	if( NULL != fb ) {
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fb->id );
		glViewport( 0, 0, fb->width, fb->height );
	} else {

		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
// TODO: How to find out viewport size ?

	}

	current_fb = fb;

	return last;

}

int      rendertargetc_Framebuffer( const Framebuffer *fb ) {

	assert( NULL != fb );
	return fb->targetc;

}

Rendertarget *
         rendertargeti_Framebuffer( const Framebuffer *fb, int i ) {

	assert( i >= 0 && i < fb->targetc );
	return fb->targetv[i];

}

#ifdef __r_target_TEST__

#include <stdlib.h>
#include <stdio.h>

#include <SDL.h>

#include "core.log.h"

#include "gl.context.h"
#include "gl.debug.h"
#include "gl.display.h"
#include "gl.shader.h"

#include "r.draw.h"

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
	                                 alphaBits, 8,
	                                 
	                                 depthBits, 24,
	                                 
	                                 doubleBuffer, 1,
	                                 requireAccel, 1,
	                                 
	                                 glMajor, 2,
	                                 glMinor, 1,
	                                 
	                                 -1 );
	if( !display )
		fatal0( "Failed to open display");

	Glcontext gl = create_Glcontext( display );
	if( !gl )
		fatal0( "Failed to create GL context" );

	bind_Glcontext( display, gl );

	// Prepare OpenGL state
	glViewport( 0, 0, width, height );
	glClearColor( 0.f, 0.f, 0.f, 0.f );
	glClear( GL_COLOR_BUFFER_BIT );
	glMatrixMode( GL_PROJECTION );
	glOrtho( -1.0*aspect, 1.0*aspect, -1.0, 1.0, -1.0, 1.0 );

	glEnable( GL_TEXTURE_2D );

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

	// render target
	Rendertarget *texture = create_Rendertexture( texture_2d,
	                                              colorAttachment0,
	                                              fragmentFormat_rgba );
	Framebuffer *fb = define_Framebuffer( width, height, 1, texture );

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

	bind_Framebuffer( fb );

	use_Program( proc, NULL );
	draw_Drawable( circle, unic, univ );

	bind_Framebuffer( NULL );

	// Now render the texture onto the back buffer
	use_Program( NULL, NULL );

	glBindTexture( GL_TEXTURE_2D, texture->id );

	glBegin( GL_QUADS );

	glTexCoord2f( 0.f, 0.f );
	glVertex2f  ( -1.f, -1.f );

	glTexCoord2f( 0.f, 1.f );
	glVertex2f  ( -1.f, 1.f );

	glTexCoord2f( 1.f, 1.f );
	glVertex2f  ( 1.f, 1.f );

	glTexCoord2f( 1.f, 0.f );
	glVertex2f  ( 1.f, -1.f );

	glEnd();

	flip_Display( display );

	glBreakpoint;
	
	getc( stdin );

	destroy_Drawable( circle );
	delete_Glcontext( gl );
	close_Display( display );

}

#endif
