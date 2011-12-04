#include <assert.h>
#include <string.h>
#include <stdio.h>
//
#include "core.alloc.h"
#include "core.log.h"
#include "core.types.h"
#include "gl.shader.h"
#include "gl.util.h"
#include "mm.region.h"

static void dumpLog( const char* log, const char* prefix ) {

	char* nl = strchr( log, '\n' );
	while( nl ) {
		
		*nl = '\0'; warning( "%s%s", prefix, log ); *nl = '\n';
		
		log = nl + 1;
		nl = strchr( log, '\n' );
		
	}
	warning( "%s%s", prefix, log );

}


// Shaders ////////////////////////////////////////////////////////////////////

Shader* compile_Shader( shaderType_e type, const char* name, const char* src ) {

	GLuint id = glCreateShader( (GLenum)type );
	if( !id ) 
		return NULL;

	// Compile
	glShaderSource( id, 1, &src, NULL ); check_GL_error;
	if( GL_NO_ERROR != gl_lastError ) {
		glDeleteShader(id); check_GL_error;
		return NULL;
	}
	glCompileShader( id ); check_GL_error;

	// Build up the Shader* object
	Shader* sh = new( NULL, Shader );
	sh->id = id;
	sh->type = type;
	sh->name = clone_string(sh, name);
	sh->src = clone_string(sh, src);
	
	// Get results
	GLint compiled; glGetShaderiv( id, GL_COMPILE_STATUS, &compiled ); check_GL_error;
	sh->compiled = GL_TRUE == compiled ? true : false;

	GLint log_length; glGetShaderiv( id, GL_INFO_LOG_LENGTH, &log_length ); check_GL_error;
	if( log_length > 1 ) {
		sh->log = alloc( sh, log_length );
		glGetShaderInfoLog( id, log_length, NULL, sh->log ); check_GL_error;
	}

	if( !sh->compiled ) {

		const char* kind = (shadeVertex == type) ? "vertex" : "fragment";
		warning( "compilation failed for %s shader '%s':", kind, sh->name );
		dumpLog( sh->log, "\t" );

	}
		
	check_GL_error;
	return sh;
}

void     delete_Shader( Shader* sh ) {

	glDeleteShader(sh->id); check_GL_error;

	memset( sh, 0, sizeof(Shader) );
	delete(sh);

}

// Shader types ///////////////////////////////////////////////////////////////

static Shader_Type get_shade_type(GLenum type, GLint length) {

	switch( type ) {
	case GL_BOOL:
		return (Shader_Type){ type, shBool, 1, { 1, 1 }, length };
	case GL_BOOL_VEC2:
		return (Shader_Type){ type, shBool, 1, { 1, 2 }, length };
	case GL_BOOL_VEC3:
		return (Shader_Type){ type, shBool, 1, { 1, 3 }, length };
	case GL_BOOL_VEC4:
		return (Shader_Type){ type, shBool, 1, { 1, 4 }, length };
	case GL_INT:
		return (Shader_Type){ type, shInt, 4, { 1, 1 }, length };
	case GL_INT_VEC2:
		return (Shader_Type){ type, shInt, 4, { 1, 2 }, length };
	case GL_INT_VEC3:
		return (Shader_Type){ type, shInt, 4, { 1, 3 }, length };
	case GL_INT_VEC4:
		return (Shader_Type){ type, shInt, 4, { 1, 4 }, length };
	case GL_FLOAT:
		return (Shader_Type){ type, shFloat, 4, { 1, 1 }, length };
	case GL_FLOAT_VEC2:
		return (Shader_Type){ type, shFloat, 4, { 1, 2 }, length };
	case GL_FLOAT_VEC3:
		return (Shader_Type){ type, shFloat, 4, { 1, 3 }, length };
	case GL_FLOAT_VEC4:
		return (Shader_Type){ type, shFloat, 4, { 1, 4 }, length };
	case GL_FLOAT_MAT2:
		return (Shader_Type){ type, shFloat, 4, { 2, 2 }, length };
	case GL_FLOAT_MAT3:
		return (Shader_Type){ type, shFloat, 4, { 3, 3 }, length };
	case GL_FLOAT_MAT4:
		return (Shader_Type){ type, shFloat, 4, { 4, 4 }, length };
	case GL_FLOAT_MAT2x3:
		return (Shader_Type){ type, shFloat, 4, { 2, 3 }, length };
	case GL_FLOAT_MAT2x4:
		return (Shader_Type){ type, shFloat, 4, { 2, 4 }, length };
	case GL_FLOAT_MAT3x2:
		return (Shader_Type){ type, shFloat, 4, { 3, 2 }, length };
	case GL_FLOAT_MAT3x4:
		return (Shader_Type){ type, shFloat, 4, { 3, 4 }, length };
	case GL_FLOAT_MAT4x2:
		return (Shader_Type){ type, shFloat, 4, { 4, 2 }, length };
	case GL_FLOAT_MAT4x3:
		return (Shader_Type){ type, shFloat, 4, { 4, 3 }, length };
	case GL_SAMPLER_1D:
		return (Shader_Type){ type, shSampler, 4, { 0, sampler1d }, length };
	case GL_SAMPLER_2D:
		return (Shader_Type){ type, shSampler, 4, { 0, sampler2d }, length };
	case GL_SAMPLER_3D:
		return (Shader_Type){ type, shSampler, 4, { 0, sampler3d }, length };
	case GL_SAMPLER_CUBE:
		return (Shader_Type){ type, shSampler, 4, { 0, samplerCube }, length };
	case GL_SAMPLER_1D_SHADOW:
		return (Shader_Type){ type, shSampler, 4, { 0, sampler1dShadow }, length };
	case GL_SAMPLER_2D_SHADOW:
		return (Shader_Type){ type, shSampler, 4, { 0, sampler2dShadow }, length };
	default:
		fatal( "Unknown shade type: %d\n", type );
		break;
	}
	// Shut the compiler up
	return (Shader_Type){ 0, 0, 0, { 0, 0 }, 0 };
}

int sizeof_Shader( Shader_Type type ) {

	switch( type.prim ) {

	case shBool:
	case shInt:
		return type.length * type.shape[0] * type.shape[1] * sizeof(int);

	case shSampler:
		return type.length * sizeof(int);

	case shFloat:
		return type.length * type.shape[0] * type.shape[1] * sizeof(float);

	default:
		fatal( "Unknown Shader_Type: %d\n", type );
		break;

	}

	return -1;

}

Shader_Arg* bind_Shader_argv( region_p R, int argc,
                              Shader_Param* params,
                              ... ) {

	pointer bindings[ argc ];
	va_list argv;

	va_start( argv, params );
	for( int i=0; i<argc; i++ )
		bindings[i] = va_arg( argv, pointer );
	va_end( argv );

	return bind_Shader_args( R, argc, params, bindings );

}

Shader_Arg* bind_Shader_args( region_p R, int argc, 
                              Shader_Param* params, 
                              pointer* bindings ) {
	
	int size = 0;

	// First figure out total size
	for( int i=0; i<argc; i++ ) {

		if( NULL == bindings[i] )
			size += sizeof(Shader_Arg) + sizeof_Shader(params[i].type);
		else
			size += sizeof(Shader_Arg);

	}

	// Allocate argv
	Shader_Arg* argv = (Shader_Arg*)ralloc( R, size );

	// Fill in bindings/initialize values
	Shader_Arg* arg = argv;
	for( int i=0; i<argc; i++ ) {

		arg->loc  = params[i].loc;
		arg->type = params[i].type;
		arg->binding = bindings[i];

		// NULL indicates an inline value, clear it to 0
		if( NULL == arg->binding )
			memset( &arg->value, 0, sizeof_Shader(arg->type) );

		// Note this is slightly delicate. argi_Shader uses the
		// fact that arg->binding is NULL in order to compute its value.
		// Hence the correct value depends on having the first 0..i
		// arguments initialized before asking for i+1
		arg = argi_Shader( argv, i+1 );

	}

	return argv;

}

Shader_Arg* argi_Shader( Shader_Arg* argv, int I ) {

	Shader_Arg* arg = argv;
	for( int i=0; i<I; i++ ) {

		if( arg->binding )
			arg = arg + 1;		
		else 
			arg = field_ofs( arg, 
			                 offsetof(Shader_Arg, value) + sizeof_Shader(arg->type),
			                 Shader_Arg );
		
	}

	return arg;

}

pointer      arg_Shader_value( Shader_Arg* arg ) {

	return arg->binding ? arg->binding : &arg->value;

}

// Attrib/Uniform helpers /////////////////////////////////////////////////////

typedef void  (*get_active_param_f)(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);
typedef GLint (*get_param_location_f)(GLuint, const GLchar*);

static int indexOf_binding( const char* what, int n, const char* strings[] ) {

	for( int i=0; i<n; i++ ) {

		if( 0 == strcmp(what, strings[i]) )
			return i;

	}
	return -1;

}

static Shader_Param* get_active_params( Program* pgm, GLint* active,
                                        GLenum active_query, 
                                        GLenum maxlen_query, 
                                        get_active_param_f get,
                                        get_param_location_f location,
                                        int       n_bindings,
                                        const char* bindings[] ) {
                                     
	GLint   N;      glGetProgramiv( pgm->id, active_query, &N );      check_GL_error;
	GLsizei maxlen; glGetProgramiv( pgm->id, maxlen_query, &maxlen ); check_GL_error;

	Shader_Param* params = new_array( pgm, Shader_Param, N );
	for( int i=0; i<N; i++ ) {

		GLchar* name = alloc(params, maxlen);

		// Get the parameter and its location
		GLenum  type; GLint size;
		get(pgm->id, i, maxlen, NULL, &size, &type, name); check_GL_error;

		int binding = indexOf_binding( name, n_bindings, bindings );
		Shader_Param* param = &params[ (binding < 0) ? i : binding ];

		param->name = name;
		param->loc  = location( pgm->id, name ); check_GL_error;
		param->type = get_shade_type(type, size);

	}

	*active = N;
	return params;
}

static Shader_Param* get_active_attribs( Program* pgm ) {
	
	return get_active_params( pgm, &pgm->n_attribs,
	                          GL_ACTIVE_ATTRIBUTES, 
	                          GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, 
	                          glGetActiveAttrib,
	                          glGetAttribLocation,
	                          0, NULL );

}

static Shader_Param* get_active_uniforms( Program* pgm, 
                                          int n_bindings,
                                          const char* bindings[] ) {
	
	return get_active_params( pgm, &pgm->n_uniforms,
	                          GL_ACTIVE_UNIFORMS, 
	                          GL_ACTIVE_UNIFORM_MAX_LENGTH, 
	                          glGetActiveUniform,
	                          glGetUniformLocation,
	                          n_bindings, bindings );

}

// Programs ///////////////////////////////////////////////////////////////////

Program* define_Program( const char* name, int n_shaders, ... ) {

	Shader* shaders[n_shaders];

	va_list argv;

	va_start( argv, n_shaders );

	for( int i=0; i<n_shaders; i++ )
		shaders[i] = va_arg(argv, Shader*);

	int n_attribs = va_arg(argv, int);
	const char* attribs[n_attribs];
	for( int i=0; i<n_attribs; i++ )
		attribs[i] = va_arg(argv, const char*);

	int n_uniforms = va_arg(argv, int);
	const char* uniforms[n_uniforms];
	for( int i=0; i<n_uniforms; i++ )
		uniforms[i] = va_arg(argv, const char*);

	va_end( argv );
	
	return build_Program( name, 
	                      n_shaders, shaders, 
	                      n_attribs, attribs,
	                      n_uniforms, uniforms );
	
}

static void fetchLog( Program* proc ) {
	
	if( proc->log )
		delete(proc->log);

	GLint log_length;

	glGetProgramiv( proc->id, GL_INFO_LOG_LENGTH, &log_length );
	if( log_length > 1 ) {

		proc->log = alloc( proc, log_length );
		glGetProgramInfoLog( proc->id, log_length, NULL, proc->log );

	} else 
		proc->log = NULL;

}

Program* build_Program( const char* name, 
                        int n_shaders, Shader* shaders[],
                        int n_attribs, const char* attribs[],
                        int n_uniforms, const char* uniforms[]) {

	GLuint id = glCreateProgram();
	if( !id )
		return NULL;

	Program* pgm = new( NULL, Program );
	pgm->id = id;
	pgm->name = clone_string(pgm, name);

	// Build
	pgm->n_shaders = n_shaders;
	pgm->shaders = new_array(pgm, Shader*, n_shaders);

	pgm->log = NULL;

	for( int i=0; i<n_shaders; i++ ) {
		glAttachShader( id, shaders[i]->id ); check_GL_error;
		pgm->shaders[i] = shaders[i];
	}

	for( int i=0; i<n_attribs; i++ )
		glBindAttribLocation( id, i, attribs[i] ); check_GL_error;

	glLinkProgram( id ); check_GL_error;

	// Get the results
	GLint built; glGetProgramiv( id, GL_LINK_STATUS, &built ); check_GL_error;
	pgm->built = GL_TRUE == built ? true : false;

	// Get the log
	fetchLog( pgm );

	// Build the attribute and uniform metadata
	if( GL_TRUE == built ) {

		pgm->attribs  = get_active_attribs( pgm );
		pgm->uniforms = get_active_uniforms( pgm, n_uniforms, uniforms );
		
	} else {

		debug( "failed to link program '%s':", pgm->name );
		dumpLog( pgm->log, "\t" );

	}

	return pgm;
}

void      delete_Program( Program* pgm ) {

	glDeleteProgram(pgm->id); check_GL_error;

	memset(pgm, 0, sizeof(Program));
	delete(pgm);

}

// Functions
Shader_Param*   attrib_Program( const Program* pgm, const char* name ) {

	for( int i=0; i<pgm->n_attribs; i++ ) {
		if( 0 == strcmp( name, pgm->attribs[i].name ) )
			return &pgm->attribs[i];
	}
	return NULL;

}

uint           attribc_Program( const Program* pgm ) {

	return pgm->n_attribs;

}

Shader_Param*  attribv_Program( const Program* pgm ) {

	return pgm->attribs;

}

Shader_Param*  attribi_Program( const Program* pgm, uint argi ) {

	assert( argi < pgm->n_attribs );
  	return &pgm->attribs[argi];

}

Shader_Param*  uniform_Program( const Program* pgm, const char* name ) {

	for( int i=0; i<pgm->n_uniforms; i++ ) {
		if( 0 == strcmp( name, pgm->uniforms[i].name ) )
			return &pgm->uniforms[i];
	}
	return NULL;

}

uint          uniformc_Program( const Program* pgm ) {

	return pgm->n_uniforms;

}

Shader_Param* uniformv_Program( const Program* pgm ) {

	return pgm->uniforms;

}

Shader_Param* uniformi_Program( const Program* pgm, uint uniformi ) {

	assert( uniformi < pgm->n_uniforms );
	return &pgm->uniforms[uniformi];

}

bool      validate_Program( Program* pgm ) {

	glValidateProgram(pgm->id); check_GL_error;

	GLint status; 

	glGetProgramiv( pgm->id, GL_VALIDATE_STATUS, &status ); check_GL_error;
	fetchLog( pgm );

	return GL_TRUE == status;

}

void          load_Program_uniforms( int argc, Shader_Arg* argv ) {

	// Load uniforms
	for( int i=0; i<argc; i++ ) {

		Shader_Arg* uniform = argi_Shader( argv, i );
		pointer       value = arg_Shader_value( uniform );
		GLint      location = uniform->loc;
		GLsizei       count = uniform->type.length;

		switch( uniform->type.gl_type ) {

		case GL_BOOL:
		case GL_INT:
		case GL_SAMPLER_1D:		
		case GL_SAMPLER_2D:
		case GL_SAMPLER_3D:
		case GL_SAMPLER_CUBE:
		case GL_SAMPLER_1D_SHADOW:
		case GL_SAMPLER_2D_SHADOW:
			glUniform1iv( location, count, value ); check_GL_error;
			break;

		case GL_BOOL_VEC2:
		case GL_INT_VEC2:
			glUniform2iv( location, count, value ); check_GL_error;
			break;

		case GL_BOOL_VEC3:
		case GL_INT_VEC3:
			glUniform3iv( location, count, value ); check_GL_error;
			break;

		case GL_BOOL_VEC4:
		case GL_INT_VEC4:
			glUniform4iv( location, count, value ); check_GL_error;
			break;

		case GL_FLOAT:
			glUniform1fv( location, count, value ); check_GL_error;
			break;
		case GL_FLOAT_VEC2:
			glUniform2fv( location, count, value ); check_GL_error;
			break;
		case GL_FLOAT_VEC3:
			glUniform3fv( location, count, value ); check_GL_error;
			break;

		case GL_FLOAT_VEC4:
			glUniform4fv( location, count, value ); check_GL_error;
			break;

		case GL_FLOAT_MAT2:
			glUniformMatrix2fv( location, count, 0, value ); check_GL_error;
			break;

		case GL_FLOAT_MAT3:
			glUniformMatrix3fv( location, count, 0, value ); check_GL_error;
			break;

		case GL_FLOAT_MAT4:
			glUniformMatrix4fv( location, count, 0, value ); check_GL_error;
			break;

		case GL_FLOAT_MAT2x3:
			glUniformMatrix2x3fv( location, count, 0, value ); check_GL_error;
			break;

		case GL_FLOAT_MAT2x4:
			glUniformMatrix2x4fv( location, count, 0, value ); check_GL_error;
			break;

		case GL_FLOAT_MAT3x2:
			glUniformMatrix2x3fv( location, count, 0, value ); check_GL_error;
			break;

		case GL_FLOAT_MAT3x4:
			glUniformMatrix3x4fv( location, count, 0, value ); check_GL_error;
			break;
		
		case GL_FLOAT_MAT4x2:
			glUniformMatrix4x2fv( location, count, 0, value ); check_GL_error;
			break;
		
		case GL_FLOAT_MAT4x3:
			glUniformMatrix4x3fv( location, count, 0, value ); check_GL_error;
			break;
		
		default:
			fatal( "Unknown uniform type: %d", uniform->type.gl_type );
			break;
			
		}
		
	}

}

void           use_Program( Program* pgm, Shader_Arg* uniforms ) {

	if( NULL == pgm ) {
		glUseProgram( 0 ); check_GL_error;
		return;
	}

	glUseProgram( pgm->id ); check_GL_error;
	if( uniforms )
		load_Program_uniforms( pgm->n_uniforms, uniforms );

}
