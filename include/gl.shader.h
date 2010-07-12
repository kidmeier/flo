#ifndef __gl_shader_h__
#define __gl_shader_h__

#include <GL/glew.h>
#include "core.types.h"

// Shader objects /////////////////////////////////////////////////////////////

enum shader_type_e {
	
	shadeVertex = GL_VERTEX_SHADER,
	shadeFragment = GL_FRAGMENT_SHADER,

};

struct shader_s;
typedef struct shader_s shader_t;
typedef shader_t* shader_p;

struct shader_s {

	GLuint             id;
	enum shader_type_e type;
	char*              name;

	char* src;

	bool  compiled;	
	char* log;

};

shader_p compile_SHADER( enum shader_type_e type, const char* name, const char* src );
void     delete_SHADER( shader_p sh );

// Shader types (describes type of attribs, uniforms) /////////////////////////

enum sh_primitive_e {

	shBool,
	shInt,
	shFloat,
	shSampler

};

enum sh_sampler_e {
	sampler1d,
	sampler2d,
	sampler3d,
	samplerCube,
	sampler1dShadow,
	sampler2dShadow
};

struct sh_type_s;
typedef struct sh_type_s sh_type_t;
struct sh_type_s {

	GLenum              gl_type;

	enum sh_primitive_e prim;
	uint                shape[2];
	uint                length;

};

int sizeof_SH( sh_type_t type );

// Parameters (uniforms or attributes) ////////////////////////////////////////

struct sh_param_s;
typedef struct sh_param_s sh_param_t;
typedef sh_param_t* sh_param_p;

struct sh_param_s {

	GLchar*   name;

	GLuint    loc;
	sh_type_t type;
       
};

// Arguments (a realization of a parameter) ///////////////////////////////////

struct sh_arg_s;
typedef struct sh_arg_s sh_arg_t;
typedef sh_arg_t* sh_arg_p;
struct sh_arg_s {

	sh_type_t type;
	byte      arg[];

};

sh_arg_p argv_SH( int argc, sh_param_p params );
sh_arg_p argi_SH( sh_arg_p argv, int I );

// Programs ///////////////////////////////////////////////////////////////////

struct program_s;
typedef struct program_s program_t;
typedef program_t* program_p;

struct program_s {

	GLuint      id;
	const char* name;

	int        n_shaders;
	shader_p*  shaders;
	GLint      n_uniforms;
	sh_param_p uniforms;
	GLint      n_attribs;
	sh_param_p attribs;

	bool  built;
	char* log;
	
};

program_p build_PROGRAM( const char* name, int n_shaders, shader_p shaders[] );
void      delete_PROGRAM( program_p pgm );

bool      validate_PROGRAM( program_p pgm );

#endif
