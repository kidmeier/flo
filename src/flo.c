#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "control.maybe.h"
#include "control.predicate.h"

#include "core.log.h"
#include "core.system.h"

#include "gl.context.h"
#include "gl.display.h"

#include "ev.channel.h"
#include "ev.core.h"
#include "ev.axis.h"
#include "ev.cursor.h"
#include "ev.button.h"
#include "ev.keyboard.h"
#include "ev.focus.h"
#include "ev.window.h"

#include "job.channel.h"
#include "job.core.h"
#include "job.control.h"

#include "math.matrix.h"
#include "math.vec.h"
#include "math.util.h"

#include "mm.region.h"

#include "phys.clock.h"

#include "r.drawable.h"
#include "r.frame.h"
#include "r.mesh.h"
#include "r.scene.h"
#include "r.state.h"
#include "r.view.h"

#include "res.core.h"
#include "res.obj.h"
#include "res.spec.h"
#define RES_SPEC "etc/res.import.spec"

#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.thread.h"

static int  tick           = 0;
static bool quit_requested = false;

declare_job( void, window_Ev_monitor, ev_channel_p evch );
declare_job( void, cursor_Ev_monitor, 

             ev_channel_p evch;
             float sensitivity;
             mat44 *view;
             float4 T;
             float  yaw;
             float  pitch );

static Rpipeline *sync_renderLoop( pointer rpipe ) {

	if( quit_requested )
		return NULL;
	
	pump_EV(tick++);
	return (Rpipeline*)rpipe;
	
}

static char *slurp( const char *path ) {

	FILE *fp = fopen( path, "rb" );

	fseek( fp, 0L, SEEK_END );
	long sz = ftell( fp );
	char *buf = malloc( sz );

	if( !buf ) {
		fclose( fp );

		fatal( "Out of memory: failed to allocate %ld bytes", sz);
		return NULL;
	}

	rewind( fp );
	fread( buf, 1, sz, fp );
	fclose( fp );

	return buf;

}

int main(int argc, char* argv[]) {
	
	region_p R = region("main");
	
	add_Res_path( "file", "${PWD}/res" );
	load_Res_spec( RES_SPEC );
	
	init_printf_MAT44();
	init_printf_FLOAT4();
	
	// Initialize SDL
	if( SDL_Init( SDL_INIT_NOPARACHUTE ) < 0 ) {
		
		fprintf(stderr, "Error: SDL_Init(): %s\n", SDL_GetError());
		exit(1);
		
	}
	
	Display* display = open_Display( "Flo",
	                                 512, 288, 0,
	                                 8, 8, 8, 8, // color bits
	                                 24, 8,      // depth-stencil bits
	                                 3, 2 );     // opengl version
	
	if( NULL == display ) {
		fatal0("Failed to open display");
		exit(1);
	}
	
	Glcontext gl = create_Glcontext( display );
	if( !gl ) {
		fatal0("Failed to create GL context");
		exit(1);
	}
	
	if( init_Jobs( cpu_count_SYS() ) < 0 )
		fatal0("Failed to initialize jobs runtime");
	if( init_EV() < 0 )
		fatal0("Failed to initialize event system");

	struct ev_channel_s* axes    = open_EV( axis_EV_adaptor );
	struct ev_channel_s* keyb    = open_EV( kbd_EV_adaptor );
	struct ev_channel_s* buttons = open_EV( button_EV_adaptor );
	struct ev_channel_s* cursor  = open_EV( cursor_EV_adaptor );
	struct ev_channel_s* focus   = open_EV( focus_EV_adaptor );
	struct ev_channel_s* window  = open_EV( window_EV_adaptor );

	// Load scene
	Resource *objRes = read_Res( "models/cylinder.mesh" );
	Mesh *obj = objRes->data;

	Shader *vertexSh = compile_Shader( shadeVertex, 
	                                   "mvp", 
	                                   slurp("art/shaders/mvp.vert") );
	Shader *fragmentSh = compile_Shader( shadeFragment, 
	                                     "flat", 
	                                     slurp("art/shaders/flat.frag") );
	Program *proc = define_Program( "default", 
	                                2, vertexSh, fragmentSh, 
	                                3, "vertex", "uv", "normal", 
	                                2, "projection", "modelView" );

	float     aspect = aspect_Display( display );

	float4  position = { 0.f, 0.f, 4.f, 1.f };
	float   yaw      = 0.f;
	float   pitch    = 0.f;
	View    view     = define_View( perspective_Lens( 60.f, aspect, 1.f, 16.f ),
	                                compose_Eye( qeuler( deg2rad(yaw), deg2rad(pitch), 0.f ),
	                                             position ) );
	int         unic = uniformc_Program(proc);
	Shader_Arg *univ = alloc_Shader_argv( R, unic, uniformv_Program(proc) );
	                   bind_Shader_argv( unic, univ, &view.lens, &view.eye );
	Drawable    *cyl = drawable_Mesh( R, obj );
	Scene        *sc = new_Scene( R );
	                   link_Scene( sc, &sc, 0xffffffff, cyl, univ );
	Rstate    rstate = {

		.blend = {
			.enabled = false
		},

		.clear = {
			.color = { 0.f, 0.f, 0.f, 1.f },
			.depth = 256.f,
			.stencil = 0
		},

		.depth = {
			.enabled = true,
			.mask    = true,
			.func    = funcLess,
			.znear   = 0.,
			.zfar    = 256.
		},

		.stencil = {
			.enabled = false
		}

	};
	Rpipeline  *rpipe = define_Rpipeline( clearColorBuffer|clearDepthBuffer,
	                                      1, new_Rpass( 0xffffffff, sc, fallacyp, proc, univ, &rstate ) );

	// Start event monitors
	typeof_Job_params( window_Ev_monitor ) window_params = { window };
	submit_Job( 0, ioBound, NULL, (jobfunc_f)window_Ev_monitor, &window_params );

	typeof_Job_params( cursor_Ev_monitor ) cursor_params = { 
		cursor, // ev_channel
		8.f,    // sensitivity
		&view.eye, // eye transform
		position, yaw, pitch // start parameters
	}; 
	submit_Job( 0, ioBound, NULL, (jobfunc_f)cursor_Ev_monitor, &cursor_params );

	// Start the render loop
	Channel *clkSink = new_Channel( sizeof(float), 1 );
	Clock       *clk = new_Clock( R, 1.f/10.f, clkSink );

	start_Clock( clk, 1.f );
	render_Frame_loop( display, clkSink, sync_renderLoop, rpipe );
	stop_Clock( clk );

	delete_Glcontext( gl );
	close_Display( display );

	rfree( R );
	return 0;

}

define_job( void, window_Ev_monitor, 

            Channel* source;
            Channel* passthru;
            ev_window_t     ev ) {

	begin_job;

	local(source)   = new_Channel( sizeof(ev_window_t), 16 );
	local(passthru) = push_EV_sink( arg(evch), local(source) );

	while( !quit_requested ) {

		readch( local(source), local(ev) );
		if( windowClosed == local(ev).what )
			quit_requested = true;
		else
			writech( local(passthru), local(ev) );
		

	}
	pop_EV_sink( arg(evch) );
	
	end_job;

}

define_job( void, cursor_Ev_monitor, 
            
            Channel *source;
            Channel *passthru;
            float    yaw;
            float    pitch;
            ev_cursor_t ev ) {

	begin_job;

	local(source)   = new_Channel( sizeof(local(ev)), 16 );
	local(passthru) = push_EV_sink( arg(evch), local(source) );

	local(yaw)   = arg(yaw);
	local(pitch) = arg(pitch);
	
	// Consume the first event which will likely have a crazy delta
	readch( local(source), local(ev) );

	while( !quit_requested ) {

		// Read a move
		readch( local(source), local(ev) );

		// Normalize the delta by the dimensions of the display
		float dx = local(ev).dX;
		float dy = local(ev).dY;
		float fx = fabs(dx) / 512.f;
		float fy = fabs(dy) / 288.f;

		// Do the rotation
		local(yaw)   = local(yaw) + arg(sensitivity)*fx*dx;
		local(pitch) = local(pitch) + arg(sensitivity)*fy*dy;

		debug( "yaw = %4.2f, pitch = %4.2f", 
		       fmodf(local(yaw), 360.f), 
		       fmodf(local(pitch), 360.f) );

		*arg(view) = compose_Eye( qeuler( deg2rad(local(yaw)), 
		                                  deg2rad(local(pitch)),
		                                  0.f ),
		                          arg(T) );

	}
	pop_EV_sink( arg(evch) );

	end_job;

}
