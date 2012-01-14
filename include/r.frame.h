#ifndef __r_frame_h__
#define __r_frame_h__

#include "control.predicate.h"

#include "core.types.h"

#include "job.channel.h"
#include "job.control.h"

#include "gl.display.h"
#include "gl.context.h"
#include "gl.shader.h"

#include "r.scene.h"
#include "r.state.h"

typedef struct Rpipeline Rpipeline;
typedef struct Rpass Rpass;

struct Rpass {

	int         id;

	Scene      *sc;
	predicate_f cull;

	Program    *proc;
	Shader_Arg *argv;

	Rstate      rstate;

};

typedef enum {

	clearColorBuffer = GL_COLOR_BUFFER_BIT,
	clearDepthBuffer = GL_DEPTH_BUFFER_BIT,
	clearStencilBUffer = GL_STENCIL_BUFFER_BIT

} clearMask;

struct Rpipeline {

	clearMask  mask;

	int    passc;
	Rpass *passv[];

};

// Procedural substrate ///////////////////////////////////////////////////////

Rpass *  new_Rpass( int id, 
                    Scene   *sc,   predicate_f cull,
                    Program *proc, Shader_Arg *argv,
                    Rstate  *state );

Rpipeline *define_Rpipeline( clearMask mask, int passc, ... );
Rpipeline    *new_Rpipeline( clearMask mask, int passc, Rpass *passv[] );
void      destroy_Rpipeline( Rpipeline *pipe );

void  render_Frame( Rpipeline *rpipe, float t0, float t, float dt );
void  render_Frame_loop( Display *dpy, Channel *clk, Rpipeline *(*sync)(pointer), pointer arg );

#endif
