#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//
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
	pointer shbuf = malloc( sizeof(Shader) + strlen(name) + 1 + strlen(src) + 1 );
	Shader* sh = shbuf;
	sh->id = id;
	sh->type = type;
	sh->name = shbuf + sizeof(Shader); strcpy( sh->name, name );//clone_string(sh, name);
	sh->src = sh->name + strlen(name) + 1; strcpy( sh->src, src );//clone_string(sh, src);
	
	// Get results
	GLint compiled; glGetShaderiv( id, GL_COMPILE_STATUS, &compiled ); check_GL_error;
	sh->compiled = GL_TRUE == compiled ? true : false;

	GLint log_length; glGetShaderiv( id, GL_INFO_LOG_LENGTH, &log_length ); check_GL_error;
	if( log_length > 1 ) {
		sh->log = malloc( log_length );
		glGetShaderInfoLog( id, log_length, NULL, sh->log ); check_GL_error;
	} else
		sh->log = NULL;

	if( !sh->compiled && sh->log ) {

		const char* kind = (shadeVertex == type) ? "vertex" : "fragment";
		warning( "compilation failed for %s shader '%s':", kind, sh->name );
		dumpLog( sh->log, "\t" );

	}
		
	check_GL_error;
	return sh;

}

void     delete_Shader( Shader* sh ) {

	glDeleteShader(sh->id); check_GL_error;

	if( sh->log )
		free( sh->log );

	memset( sh, 0, sizeof(Shader) );
	free( sh );

}

// Shader types ///////////////////////////////////////////////////////////////

static pointer get_shader_default_initializer( GLenum type ) {

	static GLint   defaultBool4[]  = { 0, 0, 0, 0 };
	static GLint   defaultInt4[]   = { 0, 0, 0, 0 };
	static GLfloat defaultFloat4[] = { 0.f, 0.f, 0.f, 1.f };
	static GLfloat defaultMat2[]   = { 1.f, 0.f, 
	                                   0.f, 1.f };
	static GLfloat defaultMat3[]   = { 1.f, 0.f, 0.f, 
	                                   0.f, 1.f, 0.f,
	                                   0.f, 0.f, 1.f };
	static GLfloat defaultMat4[]   = { 1.f, 0.f, 0.f, 0.f,
	                                   0.f, 1.f, 0.f, 0.f,
	                                   0.f, 0.f, 1.f, 0.f,
	                                   0.f, 0.f, 0.f, 1.f };

	static GLfloat defaultMat2x3[]   = { 1.f, 0.f, 0.f,
	                                     0.f, 1.f, 0.f };
	static GLfloat defaultMat3x2[]   = { 1.f, 0.f, 
	                                     0.f, 1.f, 
	                                     0.f, 0.f };
	static GLfloat defaultMat2x4[]   = { 1.f, 0.f, 0.f, 0.f,
	                                     0.f, 1.f, 0.f, 0.f };
	static GLfloat defaultMat4x2[]   = { 1.f, 0.f, 
	                                     0.f, 1.f,
	                                     0.f, 0.f, 
	                                     0.f, 0.f };

	static GLfloat defaultMat3x4[]   = { 1.f, 0.f, 0.f, 0.f,
	                                     0.f, 1.f, 0.f, 0.f,
	                                     0.f, 0.f, 1.f, 0.f };
	static GLfloat defaultMat4x3[]   = { 1.f, 0.f, 0.f, 
	                                     0.f, 1.f, 0.f,
	                                     0.f, 0.f, 1.f, 
	                                     0.f, 0.f, 0.f };

	static GLuint defaultSampler = 0;

	switch( type ) {
	case GL_BOOL:
	case GL_BOOL_VEC2:
	case GL_BOOL_VEC3:
	case GL_BOOL_VEC4:
		return &defaultBool4[0];

	case GL_INT:
	case GL_INT_VEC2:
	case GL_INT_VEC3:
	case GL_INT_VEC4:
		return &defaultInt4[0];
		
	case GL_FLOAT:
	case GL_FLOAT_VEC2:
	case GL_FLOAT_VEC3:
	case GL_FLOAT_VEC4:
		return &defaultFloat4[0];

	case GL_FLOAT_MAT2:   return &defaultMat2[0];
	case GL_FLOAT_MAT3:   return &defaultMat3[0];
	case GL_FLOAT_MAT4:   return &defaultMat4[0];
	case GL_FLOAT_MAT2x3: return &defaultMat2x3[0];
	case GL_FLOAT_MAT2x4: return &defaultMat2x4[0];
	case GL_FLOAT_MAT3x2: return &defaultMat3x2[0];
	case GL_FLOAT_MAT3x4: return &defaultMat3x4[0];
	case GL_FLOAT_MAT4x2: return &defaultMat4x2[0];
	case GL_FLOAT_MAT4x3: return &defaultMat4x3[0];

	case GL_SAMPLER_1D:
	case GL_SAMPLER_2D:
	case GL_SAMPLER_3D:
	case GL_SAMPLER_CUBE:
	case GL_SAMPLER_1D_SHADOW:
	case GL_SAMPLER_2D_SHADOW:
		return &defaultSampler;

	default:
		fatal( "Unknown shade type: %d\n", type );
		break;
	}
	// Shut the compiler up
	return NULL;

}

static Shader_Type get_shader_type(GLenum type, GLint count) {

	switch( type ) {
	case GL_BOOL:      return (Shader_Type){ type, shBool, sizeof(GLint), { 1, 1 }, count };
	case GL_BOOL_VEC2: return (Shader_Type){ type, shBool, sizeof(GLint), { 1, 2 }, count };
	case GL_BOOL_VEC3: return (Shader_Type){ type, shBool, sizeof(GLint), { 1, 3 }, count };
	case GL_BOOL_VEC4: return (Shader_Type){ type, shBool, sizeof(GLint), { 1, 4 }, count };
	case GL_INT:      return (Shader_Type){ type, shInt, sizeof(GLint), { 1, 1 }, count };
	case GL_INT_VEC2: return (Shader_Type){ type, shInt, sizeof(GLint), { 1, 2 }, count };
	case GL_INT_VEC3: return (Shader_Type){ type, shInt, sizeof(GLint), { 1, 3 }, count };
	case GL_INT_VEC4: return (Shader_Type){ type, shInt, sizeof(GLint), { 1, 4 }, count };
	case GL_FLOAT:      return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 1, 1 }, count };
	case GL_FLOAT_VEC2:	return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 1, 2 }, count };
	case GL_FLOAT_VEC3: return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 1, 3 }, count };
	case GL_FLOAT_VEC4: return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 1, 4 }, count };
	case GL_FLOAT_MAT2:   return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 2, 2 }, count };
	case GL_FLOAT_MAT3:   return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 3, 3 }, count };
	case GL_FLOAT_MAT4:   return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 4, 4 }, count };
	case GL_FLOAT_MAT2x3: return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 2, 3 }, count };
	case GL_FLOAT_MAT2x4: return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 2, 4 }, count };
	case GL_FLOAT_MAT3x2: return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 3, 2 }, count };
	case GL_FLOAT_MAT3x4: return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 3, 4 }, count };
	case GL_FLOAT_MAT4x2: return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 4, 2 }, count };
	case GL_FLOAT_MAT4x3: return (Shader_Type){ type, shFloat, sizeof(GLfloat), { 4, 3 }, count };

	case GL_SAMPLER_1D:        
		return (Shader_Type){ type, shSampler1d,       sizeof(GLuint), { 1, 1 }, count };
	case GL_SAMPLER_2D:        
		return (Shader_Type){ type, shSampler2d,       sizeof(GLuint), { 1, 1 }, count };
	case GL_SAMPLER_3D:        
		return (Shader_Type){ type, shSampler3d,       sizeof(GLuint), { 1, 1 }, count };
	case GL_SAMPLER_CUBE:      
		return (Shader_Type){ type, shSamplerCube,     sizeof(GLuint), { 1, 1 }, count };
	case GL_SAMPLER_1D_SHADOW: 
		return (Shader_Type){ type, shSampler1dShadow, sizeof(GLuint), { 1, 1 }, count };
	case GL_SAMPLER_2D_SHADOW: 
		return (Shader_Type){ type, shSampler2dShadow, sizeof(GLuint), { 1, 1 }, count };

	default:
		fatal( "Unknown shade type: %d\n", type );
		break;
	}
	// Shut the compiler up
	return (Shader_Type){ 0, 0, 0, { 0, 0 }, 0 };
}

int sizeof_Shade_Type( Shader_Type type ) {

	return type.primSize * type.shape.cols * type.shape.rows * type.count;

}

static Shader_Arg *_next_Shader_Arg_unchecked( Shader_Arg *arg );

Shader_Arg *alloc_Shader_argv( region_p R, int argc, Shader_Param *params ) {

	int size = sizeof(GLint); // Terminate the buffer w/ a -1 sentinel

	// First figure out total size
	for( int i=0; i<argc; i++ )
		size += sizeof(Shader_Arg) + sizeof_Shade_Type(params[i].type);

	// Allocate argv and initialize structure
	Shader_Arg *argv = ralloc( R, size );

	// Terminate the buffer with a negative location
	*(GLint*)((pointer)argv + size - sizeof(GLint)) = -1;

	Shader_Arg *arg = argv;
	for( int i=0; i<argc; i++ ) {

		arg->loc  = params[i].loc;
		arg->type = params[i].type;
		arg->var  = NULL;

		// Initialize the const value to some the type default
		pointer defaultValue = get_shader_default_initializer( arg->type.glType );
		int elementSize = sizeof_Shade_Type( arg->type) / arg->type.count;
		for( int j=0; j<arg->type.count; j++ )
			memcpy( arg->value + j*elementSize, defaultValue, elementSize );

		arg = _next_Shader_Arg_unchecked( arg );

	}

	return argv;

}

Shader_Arg  *bind_Shader_argv( int argc, Shader_Arg *args, ... ) {

	pointer bindings[ argc ];
	va_list argv;

	va_start( argv, args );
	for( int i=0; i<argc; i++ )
		bindings[i] = va_arg( argv, pointer );
	va_end( argv );

	return bind_Shader_args( argc, args, bindings );

}

Shader_Arg  *bind_Shader_args( int argc, Shader_Arg *argv, pointer *bindings ) {
	
	// Bind vars
	Shader_Arg* arg = argv;
	for( int i=0; i<argc; i++ ) {
		arg->var  = bindings[i];
		arg = next_Shader_Arg( arg );
	}

	return argv;

}

Shader_Arg   *set_Shader_Arg( Shader_Arg *arg, pointer value ) {

	assert( NULL != arg );
	memcpy( arg->value, value, sizeof_Shade_Type(arg->type) );

	return next_Shader_Arg( arg );

}

Shader_Arg  *bind_Shader_Arg( Shader_Arg *arg, pointer binding ) {

	assert( NULL != arg );
	arg->var = binding;

	return next_Shader_Arg( arg );

}

Shader_Arg *find_Shader_Arg( int argc, Shader_Param *params, Shader_Arg *argv, const char *name ) {

	Shader_Arg *arg = argv;
	for( int i=0; i<argc; i++ ) {

		if( 0 == strcmp( name, params[i].name ) )
			return arg;

		arg = next_Shader_Arg( arg );

	}
	return NULL;

}

Shader_Arg   *nth_Shader_Arg( Shader_Arg* argv, int N ) {

	Shader_Arg* arg = argv;
	for( int i=0; i<N; i++ )
		arg = next_Shader_Arg( arg );
	return arg;

}

//pointer   binding_Shader_Arg( Shader_Arg *arg ) {
//
//	assert( NULL != arg );
//	return arg->var;
//
//}

pointer     value_Shader_Arg( Shader_Arg* arg ) {

	assert( NULL != arg );
	return arg->var ? arg->var : arg->value;

}

static Shader_Arg *_next_Shader_Arg_unchecked( Shader_Arg *arg ) {

	assert( NULL != arg );
	return (Shader_Arg*)( (pointer)arg 
	                      + offsetof(Shader_Arg, value) 
	                      + sizeof_Shade_Type(arg->type) );

}

Shader_Arg  *next_Shader_Arg( Shader_Arg *arg ) {

	Shader_Arg *next = _next_Shader_Arg_unchecked( arg );

	if( next->loc < 0 )
		return NULL;
	else
		return next;

}

// Attrib/Uniform helpers /////////////////////////////////////////////////////

typedef void  (GLAPIENTRY *get_active_param_f)(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);
typedef GLint (GLAPIENTRY *get_param_location_f)(GLuint, const GLchar*);

static int indexOf_binding( GLuint pgmid,
                            get_param_location_f location, 
                            const char* what, 
                            int n, 
                            const char* strings[] ) {

	int skipped = 0;

	for( int i=0; i<n; i++ ) {

		if( location( pgmid, strings[i] ) < 0 ) {
			skipped++;
			continue;
		}

		if( 0 == strcmp(what, strings[i]) )
			return i - skipped;

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

	Shader_Param* params = calloc( N, sizeof(Shader_Param) );
	for( int i=0; i<N; i++ ) {

		GLchar name[ maxlen ];
		GLenum type = 0;
		GLint size =  0; 

		// Get name, size, type
		get(pgm->id, i, maxlen, NULL, &size, &type, name); check_GL_error;

		int binding = indexOf_binding( pgm->id, location, 
		                               name, 
		                               n_bindings, bindings );
		if( binding < 0 ) {
			warning( "A binding for shader parameter `%s' was given but is not active", 
			         bindings[i] );
			continue;
		}

		Shader_Param* param = &params[ binding ];

		param->name = malloc( maxlen ); strcpy( param->name, name );
		param->loc  = location( pgm->id, name ); check_GL_error;
		param->type = get_shader_type(type, size);

	}

	*active = N;
	return params;
}

static Shader_Param* get_active_attribs( Program* pgm ,
                                         int n_bindings,
                                         const char *bindings[] ) {
	
	return get_active_params( pgm, &pgm->n_attribs,
	                          GL_ACTIVE_ATTRIBUTES, 
	                          GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, 
	                          glGetActiveAttrib,
	                          glGetAttribLocation,
	                          n_bindings, bindings );

}

static Shader_Param* get_active_uniforms( Program* pgm, 
                                          int n_bindings,
                                          const char *bindings[] ) {
	
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
		free( proc->log );

	GLint log_length;

	glGetProgramiv( proc->id, GL_INFO_LOG_LENGTH, &log_length );
	if( log_length > 1 ) {

		proc->log = malloc( log_length );
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

	pointer pgmbuf = malloc( sizeof(Program) 
	                         + strlen(name)+1 
	                         + n_shaders*sizeof(Shader*) );

	Program* pgm = pgmbuf;
	pgm->id = id;
	pgm->name = pgmbuf + sizeof(Program);
	strcpy( (char*)pgm->name, name );

	// Build
	pgm->n_shaders = n_shaders;
	pgm->shaders = pgmbuf + sizeof(Program) + strlen(name) + 1;

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

		pgm->attribs  = get_active_attribs( pgm, n_attribs, attribs );
		pgm->uniforms = get_active_uniforms( pgm, n_uniforms, uniforms );
		
	} else {

		debug( "failed to link program '%s':", pgm->name );
		dumpLog( pgm->log, "\t" );

	}

	return pgm;
}

void      delete_Program( Program* pgm ) {

	glDeleteProgram(pgm->id); check_GL_error;

	if( pgm->attribs ) {
		for( int i=0; i<pgm->n_attribs; i++ )
			free( pgm->attribs[i].name );
		free( pgm->attribs );
	}

	if( pgm->uniforms ) {
		for( int i=0; i<pgm->n_uniforms; i++ )
			free( pgm->uniforms[i].name );
		free( pgm->uniforms );
	}

	if( pgm->log )
		free( pgm->log );

	memset(pgm, 0, sizeof(Program));
	free( pgm );

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

void          load_Program_uniforms( Shader_Arg* argv ) {


	// Load uniforms
	for( Shader_Arg *arg=argv; NULL!=arg; arg=next_Shader_Arg(arg) ) {

		pointer       value = value_Shader_Arg( arg );
		GLint      location = arg->loc;
		GLsizei       count = arg->type.count;

		switch( arg->type.glType ) {

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
			fatal( "Unknown uniform type: %d", arg->type.glType );
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
		load_Program_uniforms( uniforms );

}
