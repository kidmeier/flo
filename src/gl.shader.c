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

// Programs ///////////////////////////////////////////////////////////////////

static shade_type_t get_shade_type(GLenum type) {

	shade_type_t shtype;

	static const uint _1[] = { 1 };
	static const uint _2[] = { 2 };
	static const uint _3[] = { 3 };
	static const uint _4[] = { 4 };
	static const uint _22[] = { 2, 2 };
	static const uint _23[] = { 2, 3 };
	static const uint _24[] = { 2, 4 };
	static const uint _32[] = { 3, 2 };
	static const uint _33[] = { 3, 3 };
	static const uint _34[] = { 3, 4 };
	static const uint _42[] = { 4, 2 };
	static const uint _43[] = { 4, 3 };
	static const uint _44[] = { 4, 4 };

	switch( type ) {
	case GL_BOOL:
		return (shade_type_t){ SHADE_PRIMITIVE_BOOL, 1, _1 };
	case GL_BOOL_VEC2:
		return (shade_type_t){ SHADE_PRIMITIVE_BOOL, 1, _2 };
	case GL_BOOL_VEC3:
		return (shade_type_t){ SHADE_PRIMITIVE_BOOL, 1, _3 };
	case GL_BOOL_VEC4:
		return (shade_type_t){ SHADE_PRIMITIVE_BOOL, 1, _4 };
	case GL_INT:
		return (shade_type_t){ SHADE_PRIMITIVE_INT, 1, _1 };
	case GL_INT_VEC2:
		return (shade_type_t){ SHADE_PRIMITIVE_INT, 1, _2 };
	case GL_INT_VEC3:
		return (shade_type_t){ SHADE_PRIMITIVE_INT, 1, _3 };
	case GL_INT_VEC4:
		return (shade_type_t){ SHADE_PRIMITIVE_INT, 1, _4 };
	case GL_FLOAT:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 1, _1 };
	case GL_FLOAT_VEC2:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 1, _2 };
	case GL_FLOAT_VEC3:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 1, _3 };
	case GL_FLOAT_VEC4:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 1, _4 };
	case GL_FLOAT_MAT2:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 2, _22 };
	case GL_FLOAT_MAT3:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 2, _33 };
	case GL_FLOAT_MAT4:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 2, _44 };
	case GL_FLOAT_MAT2x3:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 2, _23 };
	case GL_FLOAT_MAT2x4:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 2, _24 };
	case GL_FLOAT_MAT3x2:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 2, _32 };
	case GL_FLOAT_MAT3x4:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 2, _34 };
	case GL_FLOAT_MAT4x2:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 2, _42 };
	case GL_FLOAT_MAT4x3:
		return (shade_type_t){ SHADE_PRIMITIVE_FLOAT, 2, _43 };
	case GL_SAMPLER_1D:
		return (shade_type_t){ SHADE_PRIMITIVE_SAMPLER, sampler1d, NULL };
	case GL_SAMPLER_2D:
		return (shade_type_t){ SHADE_PRIMITIVE_SAMPLER, sampler2d, NULL };
	case GL_SAMPLER_3D:
		return (shade_type_t){ SHADE_PRIMITIVE_SAMPLER, sampler3d, NULL };
	case GL_SAMPLER_CUBE:
		return (shade_type_t){ SHADE_PRIMITIVE_SAMPLER, samplerCube, NULL };
	case GL_SAMPLER_1D_SHADOW:
		return (shade_type_t){ SHADE_PRIMITIVE_SAMPLER, sampler1dShadow, NULL };
	case GL_SAMPLER_2D_SHADOW:
		return (shade_type_t){ SHADE_PRIMITIVE_SAMPLER, sampler2dShadow, NULL };
	}		
}

typedef void (*get_active_param_f)(GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);

static sh_param_p get_active_params( program_p pgm, GLint* active,
                                     GLenum active_query, GLenum maxlen_query, 
                                     get_active_param_f get ) {
                                     
	GLint N; glGetProgramiv( pgm->id, active_query, &N );
	GLint maxlen; glGetProgramiv( pgm->id, maxlen_query, &maxlen );

	sh_param_t* params = new_array( pgm, sh_param_t, N );
	for( int i=0; i<N; i++ ) {

		sh_param_p param = &params[i];

		param->loc = i;
		param->name = alloc(params, maxlen);

		// Get the parameter
		GLenum  type;
		get(pgm->id, i, maxlen, NULL, &param->size, &type, param->name);

		// Figure out its type
		param->type = get_shade_type(type);
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
