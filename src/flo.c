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

#include "in.mouse.h"

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

#include "r.skel.h"

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

declare_job( void, button_Ev_mux,

             int button;

             Ev_Channel *btnEvch;
             Ev_Channel *crsrEvch;

             Channel *crsrFlowthru );

declare_job( void, cursor_Ev_trackball, 

             Channel *source;
             float sensitivity;
             Xform *xform );

declare_job( void, cursor_Ev_look, 

             Channel *source;
             float sensitivity;
             Xform *xform );

static Rpipeline *sync_renderLoop( pointer rpipe ) {

	if( quit_requested )
		return NULL;
	
	wait_Ev();
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
	                                 2*512, 2*288, resizableDisplay,
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
	const char *model = argc > 1 ? argv[1] : "models/cylinder.mesh";
	Resource *objRes = read_Res( model );
	if( !objRes )
		fatal( "Failed to load resource: `%s'", model );
	Mesh *obj = objRes->data;
	float4 objPos = {
		-.5f * (obj->bounds.maxs.x + obj->bounds.mins.x),
		-.5f * (obj->bounds.maxs.y + obj->bounds.mins.y), 
		-.5f * (obj->bounds.maxs.z - obj->bounds.mins.z),
		1.f 
	};
	Shader *vertexSh = compile_Shader( shadeVertex, 
	                                   "mvp", 
//	                                   slurp("shaders/mvp.vert") );
	                                   slurp("shaders/goochVert.glsl") );
	Shader *fragmentSh = compile_Shader( shadeFragment, 
	                                     "flat", 
//	                                     slurp("shaders/flat.frag") );
	                                     slurp("shaders/goochFrag.glsl") );
	Program *proc = define_Program( "default", 
	                                2, vertexSh, fragmentSh, 
	                                3, "vertex", "uv", "normal", 
	                                3, "lightPos", "projection", "modelView" );

	float aspect = aspect_Display( display );
	float4 eyeQr = qeuler( 0.f, 0.f, 0.f );
	float4 eyePos = { 
		.5f * (obj->bounds.maxs.x + obj->bounds.mins.x),
		.5f * (obj->bounds.maxs.y + obj->bounds.mins.y), 
		2.f * (obj->bounds.maxs.z - obj->bounds.mins.z),
		1.f 
	};

	View    view     = define_View( R, 
	                                perspective_Lens( 60.f, 
	                                                  aspect, 
	                                                  1.f, 
	                                                  1024.f ),
	                                compose_Eye( eyeQr, eyePos ) );
	Xform   *objXform = new_Xform_m( R, view.eye, obj, &objPos );
//	Xform   *objXanchor = new_Xform_tr( R, objXform, obj, objPos );

	int         unic = uniformc_Program(proc);
	Shader_Arg *univ = bind_Shader_argv( unic, alloc_Shader_argv( R, unic, uniformv_Program(proc) ),
	                                     &eyePos,
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
			.color = { .3f, .3f, .3f, 1.f },
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

	Channel *trackballSink = new_Channel( sizeof(ev_cursor_t), 2 );
	typeof_Job_params( cursor_Ev_trackball ) trackball_params = {
		trackballSink,
		.5f,
		objXform
	};
	Handle crsrJob = submit_Job( 0, ioBound, NULL, (jobfunc_f)cursor_Ev_trackball, &trackball_params );

	typeof_Job_params( button_Ev_mux ) mux_params = {
		1,
		buttonsEv,
		cursorEv,
		trackballSink,
	};
	Handle muxJob = submit_Job( 0, ioBound, NULL, (jobfunc_f)button_Ev_mux, &mux_params );

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

	cancel_Job( crsrJob );
	cancel_Job( muxJob );

	mutex_t mutex; init_MUTEX( &mutex );
	condition_t signal; init_CONDITION( &signal );

	join_deadline_Job( 0, &mutex, &signal );

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

define_job( void, button_Ev_mux,

            Chanmux *mux;
            Channel *muxChannels[2];
            muxOp_e muxOps[2];
            uint16  muxSizes[2];
            pointer muxPtrs[2];

            Channel *btnSource;
            Channel *crsrSource;
            Channel *btnPassthru;
            Channel *crsrPassthru;

            ev_button_t btnEv;
            ev_cursor_t crsrEv;

            bool passthruPending;
            bool flowthruPending;
            uint8 pressed ) {

	begin_job;

	local(btnSource)   = new_Channel( sizeof(local(btnEv)), 2 );
	local(btnPassthru) = push_Ev_sink( arg(btnEvch), local(btnSource) );

	local(crsrSource) = new_Channel( sizeof(local(crsrEv)), 2 );
	local(crsrPassthru) = push_Ev_sink( arg(crsrEvch), local(crsrSource) );

	local(muxOps)[0] = channelRead;
	local(muxOps)[1] = channelRead;

	local(muxChannels)[0] = local(btnSource);
	local(muxChannels)[1] = local(crsrSource);

	local(muxSizes)[0] = sizeof( local(btnEv) );
	local(muxSizes)[1] = sizeof( local(crsrEv) );

	local(muxPtrs)[0] = &local(btnEv);
	local(muxPtrs)[1] = &local(crsrEv);
	
	local(mux) = new_Chanmux( 2, local(muxOps), local(muxChannels), local(muxSizes), local(muxPtrs) );

	local(flowthruPending) = false;
	local(passthruPending) = false;
	local(pressed) = false;
	while( !quit_requested ) {

		muxch( local(mux), ch ) {

			if( 0 == ch ) {
				local(pressed) = local(btnEv).which == arg(button) && local(btnEv).pressed;
				
				int captured = set_Mouse_captured( local(pressed) );
				debug( "button_Ev_mux: which=%d, pressed=%d, captured=%d", 
				       local(btnEv).which, 
				       local(pressed),
				       captured );

			}

			else {

				if( local(pressed) )
					local(flowthruPending) = true;
				else
					local(passthruPending) = true;

			}

		}

		if( local(flowthruPending) ) {
			writech( arg(crsrFlowthru), local(crsrEv) );
			local(flowthruPending) = false;
		}

		if( local(passthruPending) ) {
			writech( local(crsrPassthru), local(crsrEv) );
			local(passthruPending) = false;
		}

	}

	pop_Ev_sink( arg(btnEvch) );
	pop_Ev_sink( arg(crsrEvch) );

	destroy_Channel( local(btnSource) );
	destroy_Channel( local(crsrSource) );

	destroy_Chanmux( local(mux) );

	end_job;

}

define_job( void, cursor_Ev_trackball,
            
            float4      qr;
            ev_cursor_t ev ) {

	begin_job;

	local(qr) = qeuler( 0.f, 0.f, 0.f );
	
	while( !quit_requested ) {

		// Read a move
		readch( arg(source), local(ev) );

		// Normalize the delta by the dimensions of the display
		float dx = arg(sensitivity) * local(ev).dX;
		float dy = arg(sensitivity) * -local(ev).dY;

		// Find a perpindicular axis in the object's reference frame
		float4 qr_1 = qconj( local(qr) );
		float4 a = qrot( qr_1, (float4){ dx, dy,  1.f, 0.f } );
		float4 b = qrot( qr_1, (float4){ dx, dy, -1.f, 0.f } );
		float4 axis = vcross( a, b );

		// Angle is the length of the mouse move
		axis.w = deg2rad( sqrt( dx*dx + dy*dy ) );

//		debug( "trackball: (%6.2f, %6.2f)", dx, dy );
		local(qr) = qmul( local(qr), qaxis( axis ) );

		mat44 M = qmatrix( local(qr) );
		set_Xform( arg(xform), &M );

		// Force the update
		world_Xform( arg(xform) );

	}

	end_job;

}

define_job( void, cursor_Ev_look,
            
            float4      qr;
            ev_cursor_t ev ) {

	begin_job;

	local(qr) = qeuler( 0.f, 0.f, 0.f );
	
	while( !quit_requested ) {

		// Read a move
		readch( arg(source), local(ev) );

		// Normalize the delta by the dimensions of the display
		float dx = arg(sensitivity) * local(ev).dX;
		float dy = arg(sensitivity) * -local(ev).dY;

		debug( "look: (yaw, pitch) = (%6.2f %6.2f)", dx, dy );
		local(qr) = qmul( local(qr), qeuler( deg2rad(dx), deg2rad(dy), 0.f ) );

		mat44 M = qmatrix( local(qr) );
		set_Xform( arg(xform), &M );

		// Force the update
		world_Xform( arg(xform) );

	}
   
	end_job;

}
