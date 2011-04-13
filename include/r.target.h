#ifndef __r_target_h__
#define __r_target_h__

#include <GL/glew.h>

#include "core.types.h"
#include "r.texture.h"

typedef enum {

	colorAttachment0       = GL_COLOR_ATTACHMENT0,
	depthAttachment        = GL_DEPTH_ATTACHMENT,
	stencilAttachment      = GL_STENCIL_ATTACHMENT,
	depthStencilAttachment = GL_DEPTH_STENCIL_ATTACHMENT,

} framebufferAttachment_e;

typedef enum {

	fragmentFormat_alpha          = GL_ALPHA,
	fragmentFormat_luminance      = GL_LUMINANCE,
	fragmentFormat_luminanceAlpha = GL_LUMINANCE_ALPHA,
	fragmentFormat_rgba           = GL_RGBA,
	fragmentFormat_rgb            = GL_RGB,
	fragmentFormat_srgb           = GL_SRGB,
	fragmentFormat_srgba          = GL_SRGB_ALPHA,
	fragmentFormat_rgba_32f       = GL_RGBA32F,
	fragmentFormat_rgba_16f       = GL_RGBA16F,
	fragmentFormat_rgb_32f        = GL_RGB32F,
	fragmentFormat_rgb_16f        = GL_RGB16F,	

} colorRenderable_e;

typedef enum {

	fragmentFormat_depth        = GL_DEPTH_COMPONENT,
	fragmentFormat_depth_16     = GL_DEPTH_COMPONENT16,
	fragmentFormat_depth_32     = GL_DEPTH_COMPONENT32,
	fragmentFormat_depth_32f    = GL_DEPTH_COMPONENT32F,

	fragmentFormat_depthStencil = GL_DEPTH_STENCIL,
	
} depthRenderable_e;

typedef enum {

	fragmentFormat_stencil    = GL_STENCIL_INDEX,
	fragmentFormat_stencil_1  = GL_STENCIL_INDEX1,
	fragmentFormat_stencil_4  = GL_STENCIL_INDEX4,
	fragmentFormat_stencil_8  = GL_STENCIL_INDEX8,
	fragmentFormat_stencil_16 = GL_STENCIL_INDEX16,

} stencilRenderable_e;

typedef struct Framebuffer  Framebuffer;
typedef struct Rendertarget Rendertarget;

struct Rendertarget {

	GLenum type;
	GLuint id;

	framebufferAttachment_e attachment;
	GLenum                  format;

};

struct Framebuffer {

	GLuint id;

	GLuint width;
	GLuint height;

	int           targetc;
	Rendertarget *targetv[];

};

Rendertarget * create_Renderbuffer( framebufferAttachment_e attachment,
                                    GLenum format );
Rendertarget * create_Rendertexture( GLenum target,
                                     framebufferAttachment_e attachment,
                                     GLenum format );

void          destroy_Rendertarget( Rendertarget* targ );

Framebuffer  * define_Framebuffer( GLint width, GLint height, 
                                   int count, ... );
Framebuffer  *    new_Framebuffer( GLint width, GLint height, 
                                   int count, Rendertarget *targets[] );

void          destroy_Framebuffer( Framebuffer *fb, bool targets );

bool          validate_Framebuffer( void );
Framebuffer  *    bind_Framebuffer( Framebuffer *fb );

int      rendertargetc_Framebuffer( const Framebuffer *fb );
Rendertarget *
         rendertargeti_Framebuffer( const Framebuffer *fb, int i );

#endif
