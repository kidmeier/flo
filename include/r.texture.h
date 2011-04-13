#ifndef __r_texture_h__
#define __r_texture_h__

#include <GL/glew.h>

#include "core.types.h"

typedef enum {

	textureFormat_colorIndex     = GL_COLOR_INDEX,
	textureFormat_rgb            = GL_RGB,
	textureFormat_rgba           = GL_RGBA,
	textureFormat_red            = GL_RED,
	textureFormat_green          = GL_GREEN,
	textureFormat_blue           = GL_BLUE,
	textureFormat_alpha          = GL_ALPHA,
	textureFormat_luminance      = GL_LUMINANCE,
	textureFormat_luminanceAlpha = GL_LUMINANCE_ALPHA,
	textureFormat_depth          = GL_DEPTH_COMPONENT,
	textureFormat_depthStencil   = GL_DEPTH_STENCIL,
	
} textureFormat_e;

typedef enum {

	texelType_byte   = GL_BYTE,
	texelType_ubyte  = GL_UNSIGNED_BYTE,
	texelType_short  = GL_SHORT,
	texelType_ushort = GL_UNSIGNED_SHORT,
	texelType_int    = GL_INT,
	texelType_uint   = GL_UNSIGNED_INT,
	texelType_float  = GL_FLOAT,

} texelType_e;

typedef enum {

	texture_1d     = GL_TEXTURE_1D,
	texture_2d     = GL_TEXTURE_2D,
	texture_3d     = GL_TEXTURE_3D,

	texture_cubePositiveX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	texture_cubePositiveY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	texture_cubePositiveZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	texture_cubeNegativeX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	texture_cubeNegativeY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	texture_cubeNegativeZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,

} textureTarget_e;

typedef enum {

	texelFilter_nearest              = GL_NEAREST,
	texelFilter_linear               = GL_LINEAR,
	texelFilter_nearestMipmapNearest = GL_NEAREST_MIPMAP_NEAREST,
	texelFilter_linearMipmapNearest  = GL_LINEAR_MIPMAP_NEAREST,
	texelFilter_nearestMipmapLinear  = GL_NEAREST_MIPMAP_LINEAR,
	texelFilter_linearMipmapLinear   = GL_LINEAR_MIPMAP_LINEAR,

} texelFilter_e;

typedef enum {

	texCoord_repeat = GL_REPEAT,
	texCoord_clamp  = GL_CLAMP,

} texCoordWrap_e;

typedef struct TexParams TexParams;

struct TexParams {

	GLenum minFilter;
	GLenum magFilter;

	GLenum wrap_s;
	GLenum wrap_t;
	GLenum wrap_r;

	bool   genMipmaps;

};

#endif
