#include "core.log.h"
#include "r.frame.h"
#include "r.scene.h"
#include "time.core.h"
#include "sync.thread.h"

Rpass *  new_Rpass( int id, 
                    Scene *sc,     predicate_f cull, 
                    Program *proc, Shader_Arg *argv,
                    Rstate  *rstate ) {

	Rpass *pass = malloc( sizeof(Rpass) );

	pass->id     = id;
	pass->sc     = sc;
	pass->cull   = cull;
	pass->proc   = proc;
	pass->argv   = argv;
	pass->rstate = *rstate;

    return pass;

}

Rpipeline *define_Rpipeline( clearMask mask, int passc, ... ) {

	Rpass *passv[ passc ];
	va_list argv;

	va_start( argv, passc );
	for( int i=0; i<passc; i++ )
		passv[i] = va_arg( argv, Rpass* );
	va_end( argv );

	return new_Rpipeline( mask, passc, passv );

}

Rpipeline    *new_Rpipeline( clearMask mask, int passc, Rpass *passv[] ) {

	assert( passc > 0 );

	Rpipeline *pipe = malloc( sizeof(Rpipeline) + passc * sizeof(Rpass*) );
	if( !pipe )
		return NULL;

	pipe->mask = mask;
	pipe->passc = passc;
	memcpy( &pipe->passv[0], &passv[0], passc * sizeof(Rpass*) );

	return pipe;

}

void destroy_Rpipeline( Rpipeline *pipe ) {

	for( int i=0; i<pipe->passc; i++ )
		free( pipe->passv[ i ] );
	free( pipe );


}

void render_Frame( Rpipeline *rpipe, float t0, float t, float dt ) {
    
	debug( "RENDER: t0=%9.5f\tt=%9.5f\tdt=%9.5f ", t0, t, dt );

	glClear( rpipe->mask );
	for( int pass=0; pass<(*rpipe).passc; pass++ ) {
        
		Rpass* rpass = (*rpipe).passv[pass];

		apply_Rstate( &(*rpass).rstate );
		use_Program( (*rpass).proc, (*rpass).argv );
		draw_Scene( t0, t, dt, 
		            (*rpass).sc,
		            (*rpass).id,
		            (*rpass).cull );
	}
	
}

void render_Frame_loop( Display *dpy, Channel *clk, Rpipeline *(*sync)(pointer), pointer arg ) {

	float      t0; // time of previous tick
	float      t;  // time of current tick
	float      tn; // time of next tick
	
	// We need two samples to be able to begin interpolating
	while( channelBlocked == try_read_Channel( clk, sizeof(t0), &t0 ) )
		yield_THREAD();
	
	while( channelBlocked == try_read_Channel( clk, sizeof(t), &t ) )
		yield_THREAD();
	
	debug( "render_Frame_loop: start: t0=%9.5f t=%9.5f", t0, t );

	usec_t base = microseconds();
	while( 1 ) {

		// Sync - read the next frame
		Rpipeline *rpipe = sync( arg );
		if( NULL == rpipe )
			return;
			
		// Calculate the amount of real time elapsed since last frame
		float elapsed = (float)(microseconds() - base) / usec_perSecond;

		// Calculate interpolation amount; this is simply the fraction of
		// the interval [t0,t] that has passed since the last clock tick 
		//
		// Note that this may go above 1.f; consuming functions must either
		// support extrapolation or clamp to 1.f themselves.
		float dt = elapsed / (t - t0);

		render_Frame( rpipe, t0, t, dt );
		flip_Display( dpy );
		
		// Tick?
		if( try_read_Channel( clk, sizeof(tn), &tn ) > 0 ) {

			t0 = t;
			t  = tn;			
			base = microseconds();

		}

	}

}
