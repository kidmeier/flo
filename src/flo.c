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
#include "r.xform.h"

#include "res.core.h"
#include "res.obj.h"
#include "res.spec.h"
#define RES_SPEC "etc/res.import.spec"

#include "sync.condition.h"
#include "sync.mutex.h"
#include "sync.thread.h"

static int  tick           = 0;
static bool quit_requested = false;

declare_job( void, window_Ev_monitor, Display *dpy; Xform *proj; Ev_Channel *evch );
declare_job( void, cursor_Ev_trackball, 

             Ev_Channel *evch;
             float sensitivity;
             Xform *xform );

static Rpipeline *sync_renderLoop( pointer rpipe ) {

	if( quit_requested )
		return NULL;
	
	pump_Ev(tick++);
	return (Rpipeline*)rpipe;
	
}

static char *slurp( const char *path ) {

	FILE *fp = fopen( path, "rb" );

	fseek( fp, 0L, SEEK_END );
	long sz = ftell( fp );
	char *buf = malloc( sz+1 );

	if( !buf ) {
		fclose( fp );

		fatal( "Out of memory: failed to allocate %ld bytes", sz);
		return NULL;
	}

	rewind( fp );
	fread( buf, 1, sz, fp );
	fclose( fp );

	buf[ sz ] = '\0';
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
	                                 512, 288, resizableDisplay,
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
	if( init_Ev() < 0 )
		fatal0("Failed to initialize event system");

	Ev_Channel* axesEv    = open_Ev( axis_Ev_adaptor );
	Ev_Channel* keybEv    = open_Ev( kbd_Ev_adaptor );
	Ev_Channel* buttonsEv = open_Ev( button_Ev_adaptor );
	Ev_Channel* cursorEv  = open_Ev( cursor_Ev_adaptor );
	Ev_Channel* focusEv   = open_Ev( focus_Ev_adaptor );
	Ev_Channel* windowEv  = open_Ev( window_Ev_adaptor );

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

	float aspect = aspect_Display( display );
	float4 eyeQr = qeuler( 0.f, 0.f, 0.f );
	float4 eyePos = { 0.f, 0.f, 4.f, 1.f };

	View    view     = define_View( R, 
	                                perspective_Lens( 60.f, aspect, 1.f, 512.f ),
	                                compose_Eye( eyeQr, eyePos ) );
	Xform   *objXform = new_Xform_m( R, view.eye, obj, &identity_MAT44 );

	int         unic = uniformc_Program(proc);
	Shader_Arg *univ = bind_Shader_argv( unic, alloc_Shader_argv( R, unic, uniformv_Program(proc) ),
	                                     object_Xform( view.lens ), // projection
	                                     world_Xform( objXform ) ); // modelView
	Drawable    *cyl = drawable_Mesh( R, obj );
	Scene        *sc = new_Scene( R );
	                   link_Scene( sc, &sc, 0xffffffff, cyl, univ );
	Rstate    rstate = {

		.blend = {
			.enabled = false
		},

		.clear = {
			.color = { 0.f, 0.f, 0.f, 1.f },
			.depth = 1024.f,
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
	typeof_Job_params( window_Ev_monitor ) window_params = { display, view.lens, windowEv };
	submit_Job( 0, ioBound, NULL, (jobfunc_f)window_Ev_monitor, &window_params );

	typeof_Job_params( cursor_Ev_trackball ) cursor_params = { 
		cursorEv, // ev_channel
		8.f,    // sensitivity
		objXform
	}; 
	submit_Job( 0, ioBound, NULL, (jobfunc_f)cursor_Ev_trackball, &cursor_params );

	// Start the render loop
	Channel *clkSink = new_Channel( sizeof(float), 1 );
	Clock       *clk = new_Clock( R, 1.f/10.f, clkSink );

	start_Clock( clk, 1.f );
	render_Frame_loop( display, clkSink, sync_renderLoop, rpipe );
	stop_Clock( clk );

	delete_Shader( vertexSh );
	delete_Shader( fragmentSh );
	delete_Program( proc );

	close_Ev( axesEv );
	close_Ev( keybEv );
	close_Ev( buttonsEv );
	close_Ev( cursorEv );
	close_Ev( focusEv );
	close_Ev( windowEv );

	delete_Glcontext( gl );
	close_Display( display );

	shutdown_Jobs();

	rfree( R );
	return 0;

}

define_job( void, window_Ev_monitor, 

            Channel* source;
            Channel* passthru;
            ev_window_t     ev ) {

	begin_job;

	local(source)   = new_Channel( sizeof(ev_window_t), 16 );
	local(passthru) = push_Ev_sink( arg(evch), local(source) );

	while( !quit_requested ) {

		readch( local(source), local(ev) );
		
		int width = local(ev).size.width;
		int height = local(ev).size.height;
		int x = local(ev).position.x;
		int y = local(ev).position.y;
		mat44 lens = perspective_Lens( 60.f, aspect_Display( arg(dpy) ), 1.f, 512.f );

		if( windowClosed == local(ev).what ) {

			debug0( "windowClosed: quit requested" );
			quit_requested = true;

		} else if( windowMinimized == local(ev).what ) {

			debug( "windowMinimized: %d x %d @ (%d, %d)", width, height, x, y );
			resize_Display( arg(dpy), width, height );

		} else if( windowMaximized == local(ev).what ) {

			debug( "windowMaximized: %d x %d @ (%d, %d)", width, height, x, y );
			resize_Display( arg(dpy), width, height );
			
		} else if ( windowResized == local(ev).what ) {

			set_Xform( arg(proj), &lens );
			resize_Display( arg(dpy), width, height );

			debug( "windowResized: %d x %d @ (%d, %d)", width, height, x, y );
			
		} else if ( windowRestored == local(ev).what ) {

			debug( "windowRestored: %d x %d @ (%d, %d)", width, height, x, y );
			resize_Display( arg(dpy), width, height );
			
		} else {

			writech( local(passthru), local(ev) );

		}
		

	}
	pop_Ev_sink( arg(evch) );
	
	end_job;

}

define_job( void, cursor_Ev_trackball,
            
            Channel *source;
            Channel *passthru;
            float4   qr;
            ev_cursor_t ev ) {

	begin_job;

	local(source)   = new_Channel( sizeof(local(ev)), 16 );
	local(passthru) = push_Ev_sink( arg(evch), local(source) );

	local(qr) = qeuler( 0.f, 0.f, 0.f );
	
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
		float yaw   = arg(sensitivity) * fx * dx;
		float pitch = arg(sensitivity) * fy * dy;
		
		local(qr) = qmul( local(qr), qeuler( deg2rad(yaw), deg2rad(pitch), 0.f ) );

		mat44 M = qmatrix( local(qr) );
		set_Xform( arg(xform), &M );

		// Force the update
		world_Xform( arg(xform) );

	}
	pop_Ev_sink( arg(evch) );
	destroy_Channel( local(source) );
   
	end_job;

}
