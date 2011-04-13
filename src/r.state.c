#include "r.state.h"

void query_Rstate_blend  ( Rstate_blend* out ) {

	out->enabled = (GL_TRUE == glIsEnabled( GL_BLEND ));

	glGetIntegerv( GL_BLEND_SRC_RGB  , (GLint*)&out->srcColor );
	glGetIntegerv( GL_BLEND_DST_RGB  , (GLint*)&out->dstColor );
	glGetIntegerv( GL_BLEND_SRC_ALPHA, (GLint*)&out->srcAlpha );
	glGetIntegerv( GL_BLEND_DST_ALPHA, (GLint*)&out->dstAlpha );

	glGetIntegerv( GL_BLEND_EQUATION_RGB  , (GLint*)&out->colorFunc );
	glGetIntegerv( GL_BLEND_EQUATION_ALPHA, (GLint*)&out->alphaFunc );

	glGetFloatv( GL_BLEND_COLOR, (GLfloat*)&out->constColor );

}

void query_Rstate_clear( Rstate_clear* clear ) {

	glGetFloatv  ( GL_COLOR_CLEAR_VALUE   , (GLfloat*)&clear->color  );
	glGetDoublev ( GL_DEPTH_CLEAR_VALUE   , (GLdouble*)&clear->depth );
	glGetIntegerv( GL_STENCIL_CLEAR_VALUE, (GLint*)&clear->stencil   );

}

void query_Rstate_depth  ( Rstate_depth* out ) {

	double range[2];

	out->enabled = (GL_TRUE == glIsEnabled( GL_DEPTH_TEST ));

	glGetIntegerv( GL_DEPTH_WRITEMASK, (GLint*)&out->mask );
	glGetIntegerv( GL_DEPTH_FUNC     , (GLint*)&out->func );
	glGetDoublev ( GL_DEPTH_RANGE    , range );

	out->znear = range[0]; out->zfar  = range[1];

}

void query_Rstate_stencil( Rstate_stencil* out ) {

	out->enabled = (GL_TRUE == glIsEnabled( GL_STENCIL_TEST ));

	glGetIntegerv( GL_STENCIL_REF                 , (GLint*)&out->frontRef   );
	glGetIntegerv( GL_STENCIL_VALUE_MASK          , (GLint*)&out->frontMask  );
	glGetIntegerv( GL_STENCIL_FUNC                , (GLint*)&out->frontFunc  );

	glGetIntegerv( GL_STENCIL_BACK_REF            , (GLint*)&out->backRef    );
	glGetIntegerv( GL_STENCIL_BACK_VALUE_MASK     , (GLint*)&out->backMask   );
	glGetIntegerv( GL_STENCIL_BACK_FUNC           , (GLint*)&out->backFunc   );

	glGetIntegerv( GL_STENCIL_FAIL                , (GLint*)&out->frontFail  );
	glGetIntegerv( GL_STENCIL_PASS_DEPTH_PASS     , (GLint*)&out->frontZpass );
	glGetIntegerv( GL_STENCIL_PASS_DEPTH_FAIL     , (GLint*)&out->frontZfail );

	glGetIntegerv( GL_STENCIL_BACK_FAIL           , (GLint*)&out->backFail   );
	glGetIntegerv( GL_STENCIL_BACK_PASS_DEPTH_PASS, (GLint*)&out->backZpass  );
	glGetIntegerv( GL_STENCIL_BACK_PASS_DEPTH_FAIL, (GLint*)&out->backZfail  );

}

void query_Rstate( Rstate* out ) {

	query_Rstate_blend( &out->blend );
	query_Rstate_depth( &out->depth );
	query_Rstate_stencil( &out->stencil );

}

void apply_Rstate_blend( const Rstate_blend* blend ) {

	if( blend->enabled ) {

		glEnable( GL_BLEND );
		glBlendFuncSeparate( blend->srcColor, 
		                     blend->dstColor,
		                     blend->srcAlpha,
		                     blend->dstAlpha );

		glBlendEquationSeparate( blend->colorFunc,
		                         blend->alphaFunc );

		glBlendColor( blend->constColor.x,
		              blend->constColor.y,
		              blend->constColor.z,
		              blend->constColor.w );

	} else
		glDisable( GL_BLEND );

}

void apply_Rstate_clear( const Rstate_clear* clear ) {

	glClearColor( clear->color.x,
	              clear->color.y, 
	              clear->color.z, 
	              clear->color.w );

	glClearDepth( clear->depth );
	glClearStencil( clear->stencil );

}

void apply_Rstate_depth( const Rstate_depth* depth ) {

	if( depth->enabled ) {

		glEnable( GL_DEPTH_TEST );
		glDepthFunc( depth->func );

	} else 
		glDisable( GL_DEPTH_TEST );

	if( depth->mask ) {

		glDepthMask( GL_TRUE );
		glDepthRange( depth->znear, depth->zfar );

	} else
		glDepthMask( GL_FALSE );

}

void apply_Rstate_stencil( const Rstate_stencil* stencil ) {

	if( stencil->enabled ) {

		glEnable( GL_STENCIL_TEST );
		glStencilFuncSeparate( GL_FRONT, 
		                       stencil->frontFunc,
		                       stencil->frontRef,
		                       stencil->frontMask );
		glStencilFuncSeparate( GL_BACK, 
		                       stencil->backFunc,
		                       stencil->backRef,
		                       stencil->backMask );

		glStencilOpSeparate( GL_FRONT,
		                     stencil->frontFail,
		                     stencil->frontZpass,
		                     stencil->frontZfail );

		glStencilOpSeparate( GL_BACK,
		                     stencil->backFail,
		                     stencil->backZpass,
		                     stencil->backZfail );

	} else
		glDisable( GL_STENCIL_TEST );

}

void apply_Rstate( const Rstate* in ) {

	apply_Rstate_blend( &in->blend );
	apply_Rstate_clear( &in->clear );
	apply_Rstate_depth( &in->depth );
	apply_Rstate_stencil( &in->stencil );

}
