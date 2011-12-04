#ifndef __r_state_h__
#define __r_state_h__

#include <GL/glew.h>
#include "core.types.h"
#include "math.vec.h"

typedef enum {

	funcAdd             = GL_FUNC_ADD,              // a + b
	funcSubtract        = GL_FUNC_SUBTRACT,         // a - b
	funcReverseSubtract = GL_FUNC_REVERSE_SUBTRACT, // b - a
	funcMin             = GL_MIN,                   // min(a,b)
	funcMax             = GL_MAX,                   // max(a,b)

} blendFunc;

typedef enum {

	blendZero             = GL_ZERO,
	blendOne              = GL_ONE,
	blendSrcColour        = GL_SRC_COLOR,
	blendInvSrcColour     = GL_ONE_MINUS_SRC_COLOR,
	blendDstColour        = GL_DST_COLOR,
	blendInvDstColour     = GL_ONE_MINUS_DST_COLOR,
	blendSrcAlpha         = GL_SRC_ALPHA,
	blendInvSrcAlpha      = GL_ONE_MINUS_SRC_ALPHA,
	blendDstAlpha         = GL_DST_ALPHA,
	blendInvDstAlph       = GL_ONE_MINUS_DST_ALPHA,
	blendConstColour      = GL_CONSTANT_COLOR,
	blendInvConstColour   = GL_ONE_MINUS_CONSTANT_COLOR,
	blendConstAlpha       = GL_CONSTANT_ALPHA,
	blendInvConstAlpha    = GL_ONE_MINUS_CONSTANT_ALPHA,
	blendSrcAlphaSaturate = GL_SRC_ALPHA_SATURATE,

} blendFactor;

typedef enum {

	funcNever    = GL_NEVER,
	funcAlways   = GL_ALWAYS,
	funcLess     = GL_LESS,
	funcLequal   = GL_LEQUAL,
	funcGreater  = GL_GREATER,
	funcGrequal  = GL_GEQUAL,
	funcNotEqual = GL_NOTEQUAL,

} compareFunc;

typedef enum {

	stencilKeep     = GL_KEEP,
	stencilZero     = GL_ZERO,
	stencilReplace  = GL_REPLACE,
	stencilIncr     = GL_INCR,
	stencilIncrWrap = GL_INCR_WRAP,
	stencilDecr     = GL_DECR,
	stencilDecrWrap = GL_DECR_WRAP,
	stencilInvert   = GL_INVERT,

} stencilOp;

typedef enum {

	faceFront        = GL_FRONT,
	faceBack         = GL_BACK,
	faceFrontAndBack = GL_FRONT_AND_BACK,

} faceFunc;

typedef struct Rstate Rstate;
typedef struct Rstate_blend Rstate_blend;
typedef struct Rstate_clear Rstate_clear;
typedef struct Rstate_depth Rstate_depth;
typedef struct Rstate_stencil Rstate_stencil;

struct Rstate_blend {

	bool        enabled;
	blendFactor srcColor;
	blendFactor dstColor;
	
	blendFactor srcAlpha;
	blendFactor dstAlpha;
	
	blendFunc   colorFunc;
	blendFunc   alphaFunc;

	float4      constColor;
	
};

struct Rstate_clear {

	float4 color;
	double depth;
	int    stencil;

};

struct Rstate_depth {

	bool        enabled;
	
	bool        mask;
	compareFunc func;
	
	double      znear;
	double      zfar;
	
};

struct Rstate_stencil {
	
	bool        enabled;
	
	int         frontRef;
	unsigned    frontMask;
	compareFunc frontFunc;

	int         backRef;
	unsigned    backMask;
	compareFunc backFunc;
	
	stencilOp   frontFail, frontZpass, frontZfail;
	stencilOp   backFail , backZpass , backZfail;
	
};

struct Rstate {

	Rstate_blend   blend;
	Rstate_clear   clear;
	Rstate_depth   depth;
	Rstate_stencil stencil;

};

void query_Rstate_blend  ( Rstate_blend* blend );
void query_Rstate_clear  ( Rstate_clear* clear );
void query_Rstate_depth  ( Rstate_depth* depth );
void query_Rstate_stencil( Rstate_stencil* stencil );
void query_Rstate( Rstate* out );

void apply_Rstate_blend  ( const Rstate_blend* blend );
void apply_Rstate_clear  ( const Rstate_clear* clear );
void apply_Rstate_depth  ( const Rstate_depth* depth );
void apply_Rstate_stencil( const Rstate_stencil* stencil );
void apply_Rstate        ( const Rstate* in );

#endif
