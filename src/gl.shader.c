#include <string.h>
//
#include "core.alloc.h"
#include "gl.shader.h"

// Shaders ////////////////////////////////////////////////////////////////////

shader_p compile_SHADER( enum shader_type_e type, const char* name, const char* src ) {

	GLuint id = glCreateShader( (GLenum)type );
	if( !id ) 
		return NULL;

	// Compile
	glShaderSource( id, 1, &src, NULL );
	if( GL_NO_ERROR != glGetError() ) {
		glDeleteShader(id);
		return NULL;
	}
	glCompileShader( id );

	// Build up the shader_p object
	shader_p sh = new( NULL, shader_t );
	sh->id = id;
	sh->type = type;
	sh->name = clone_string(sh, name);
	sh->src = clone_string(sh, src);
	
	// Get results
	GLint compiled; glGetShaderiv( id, GL_COMPILE_STATUS, &compiled );
	sh->compiled = GL_TRUE == compiled ? true : false;

	GLint log_length; glGetShaderiv( id, GL_INFO_LOG_LENGTH, &log_length );
	if( log_length ) {
		sh->log = alloc( sh, log_length );
		glGetShaderInfoLog( id, log_length, NULL, sh->log );
	}
		
	return sh;
}

void     delete_SHADER( shader_p sh ) {

	glDeleteShader(sh->id);

	memset( sh, 0, sizeof(shader_t) );
	delete(sh);

}

// Shader types ///////////////////////////////////////////////////////////////

static sh_type_t get_shade_type(GLenum type, GLint length) {

	sh_type_t shtype;

	switch( type ) {
	case GL_BOOL:
		return (sh_type_t){ type, shBool, { 1, 1 }, length };
	case GL_BOOL_VEC2:
		return (sh_type_t){ type, shBool, { 1, 2 }, length };
	case GL_BOOL_VEC3:
		return (sh_type_t){ type, shBool, { 1, 3 }, length };
	case GL_BOOL_VEC4:
		return (sh_type_t){ type, shBool, { 1, 4 }, length };
	case GL_INT:
		return (sh_type_t){ type, shInt, { 1, 1 }, length };
	case GL_INT_VEC2:
		return (sh_type_t){ type, shInt, { 1, 2 }, length };
	case GL_INT_VEC3:
		return (sh_type_t){ type, shInt, { 1, 3 }, length };
	case GL_INT_VEC4:
		return (sh_type_t){ type, shInt, { 1, 4 }, length };
	case GL_FLOAT:
		return (sh_type_t){ type, shFloat, { 1, 1 }, length };
	case GL_FLOAT_VEC2:
		return (sh_type_t){ type, shFloat, { 1, 2 }, length };
	case GL_FLOAT_VEC3:
		return (sh_type_t){ type, shFloat, { 1, 3 }, length };
	case GL_FLOAT_VEC4:
		return (sh_type_t){ type, shFloat, { 1, 4 }, length };
	case GL_FLOAT_MAT2:
		return (sh_type_t){ type, shFloat, { 2, 2 }, length };
	case GL_FLOAT_MAT3:
		return (sh_type_t){ type, shFloat, { 3, 3 }, length };
	case GL_FLOAT_MAT4:
		return (sh_type_t){ type, shFloat, { 4, 4 }, length };
	case GL_FLOAT_MAT2x3:
		return (sh_type_t){ type, shFloat, { 2, 3 }, length };
	case GL_FLOAT_MAT2x4:
		return (sh_type_t){ type, shFloat, { 2, 4 }, length };
	case GL_FLOAT_MAT3x2:
		return (sh_type_t){ type, shFloat, { 3, 2 }, length };
	case GL_FLOAT_MAT3x4:
		return (sh_type_t){ type, shFloat, { 3, 4 }, length };
	case GL_FLOAT_MAT4x2:
		return (sh_type_t){ type, shFloat, { 4, 2 }, length };
	case GL_FLOAT_MAT4x3:
		return (sh_type_t){ type, shFloat, { 4, 3 }, length };
	case GL_SAMPLER_1D:
		return (sh_type_t){ type, shSampler, { 0, sampler1d }, length };
	case GL_SAMPLER_2D:
		return (sh_type_t){ type, shSampler, { 0, sampler2d }, length };
	case GL_SAMPLER_3D:
		return (sh_type_t){ type, shSampler, { 0, sampler3d }, length };
	case GL_SAMPLER_CUBE:
		return (sh_type_t){ type, shSampler, { 0, samplerCube }, length };
	case GL_SAMPLER_1D_SHADOW:
		return (sh_type_t){ type, shSampler, { 0, sampler1dShadow }, length };
	case GL_SAMPLER_2D_SHADOW:
		return (sh_type_t){ type, shSampler, { 0, sampler2dShadow }, length };
	}		
}

int sizeof_SH( sh_type_t type ) {

	int base_sz = 0;
	switch( type.prim ) {
	case shBool:
	case shInt:
		return type.length * type.shape[0] * type.shape[1] * sizeof(int);
	case shSampler:
		return type.length * sizeof(int);
		break;
	case shFloat:
		return type.length * type.shape[0] * type.shape[1] * sizeof(float);
		break;
	}

}

sh_arg_p argv_SH( int argc, sh_param_p params ) {

	int size = 0;

	// First figure out the total size
	for( int i=0; i<argc; i++ )
		size += sizeof(sh_arg_t) + sizeof_SH(params[i].type);

	// Allocate the argv
	sh_arg_p argv = (sh_arg_p)alloc(NULL,size);

	// Fill in the sh_type_t descriptors
	sh_arg_p arg = argv;
	for( int i=0; i<argc; i++ ) {

		arg->type = params[i].type;
		arg = argi_SH( argv, i );

	}

	return argv;

}

sh_arg_p argi_SH( sh_arg_p argv, int I ) {

	sh_arg_p arg = argv;
	for( int i=0; i<I; i++ )
		arg = field_ofs( arg, ofs_of(sh_arg_t, arg) + sizeof_SH(arg->type), sh_arg_t );

	return arg;

}

// Attrib/Uniform helpers /////////////////////////////////////////////////////

typedef void (*get_active_param_f)(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);

static sh_param_p get_active_params( program_p pgm, GLint* active,
                                     GLenum active_query, GLenum maxlen_query, 
                                     get_active_param_f get ) {
                                     
	GLint N;      glGetProgramiv( pgm->id, active_query, &N );
	GLint maxlen; glGetProgramiv( pgm->id, maxlen_query, &maxlen );

	sh_param_t* params = new_array( pgm, sh_param_t, N );
	for( int i=0; i<N; i++ ) {

		sh_param_p param = &params[i];

		param->loc = i;
		param->name = alloc(params, maxlen);

		// Get the parameter
		GLenum  type; GLint size;
		get(pgm->id, i, maxlen, NULL, &size, &type, param->name);

		// Figure out its type
		param->type = get_shade_type(type, size);
	}

	*active = N;
	return params;
}

static sh_param_p get_active_attribs( program_p pgm ) {
	
	return get_active_params( pgm, &pgm->n_attribs,
	                          GL_ACTIVE_ATTRIBUTES, 
	                          GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, 
	                          glGetActiveAttrib );

}

static sh_param_p get_active_uniforms( program_p pgm ) {
	
	return get_active_params( pgm, &pgm->n_uniforms,
	                          GL_ACTIVE_UNIFORMS, 
	                          GL_ACTIVE_UNIFORM_MAX_LENGTH, 
	                          glGetActiveUniform );

}

// Programs ///////////////////////////////////////////////////////////////////

program_p build_PROGRAM( const char* name, int n_shaders, shader_p shaders[] ) {

	GLuint id = glCreateProgram();
	if( !id )
		return NULL;

	program_p pgm = new( NULL, program_t );
	pgm->id = id;
	pgm->name = clone_string(pgm, name);

	// Build
	pgm->n_shaders = n_shaders;
	pgm->shaders = new_array(pgm, shader_p, n_shaders);
	for( int i=0; i<n_shaders; i++ ) {
		glAttachShader( id, shaders[i]->id );
		pgm->shaders[i] = shaders[i];
	}
	glLinkProgram( id );

	// Get the results
	GLint built; glGetProgramiv( id, GL_LINK_STATUS, &built );
	pgm->built = GL_TRUE == built ? true : false;

	// Get the log
	GLint log_length;	glGetProgramiv( id, GL_INFO_LOG_LENGTH, &log_length );
	if( log_length ) {
		pgm->log = alloc( pgm, log_length );
		glGetProgramInfoLog( id, log_length, NULL, pgm->log );
	}

	if( GL_TRUE == built ) {

		pgm->attribs = get_active_attribs( pgm );
		pgm->uniforms = get_active_uniforms( pgm );
		
	}

	return pgm;
}

void      delete_PROGRAM( program_p pgm ) {

	glDeleteProgram(pgm->id);

	memset(pgm, 0, sizeof(program_t));
	delete(pgm);
}

bool      validate_PROGRAM( program_p pgm ) {

	glValidateProgram(pgm->id);
	

	GLint status; glGetProgramiv( pgm->id, GL_VALIDATE_STATUS, &status );
	return GL_TRUE == status;

}

void      use_PROGRAM( program_p pgm, sh_arg_p uniforms, sh_arg_p attribs ) {

	

}
