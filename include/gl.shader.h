#ifndef __gl_shader_h__
#define __gl_shader_h__

#include <GL/glew.h>
#include "core.types.h"
#include "mm.region.h"

// Shader objects /////////////////////////////////////////////////////////////

typedef enum  {
	
	shadeVertex = GL_VERTEX_SHADER,
	shadeFragment = GL_FRAGMENT_SHADER,

} shaderType_e;

typedef struct Shader Shader;

struct Shader {

	GLuint       id;
	shaderType_e type;
	char*        name;

	char*        src;

	bool         compiled;	
	char*        log;

};

Shader* compile_Shader( shaderType_e type, const char* name, const char* src );
void     delete_Shader( Shader* sh );

// Shader types (describes type of attribs, uniforms) /////////////////////////

typedef enum {

	shBool,
	shInt,
	shFloat,
	shSampler

} shaderPrimitive_e;

typedef enum {
	sampler1d,
	sampler2d,
	sampler3d,
	samplerCube,
	sampler1dShadow,
	sampler2dShadow
} shaderSampler_e;

typedef struct Shader_Type Shader_Type;

struct Shader_Type {

	GLenum              gl_type;

	shaderPrimitive_e   prim;
	uint8               shape[2];
	uint8               length;

};

int sizeof_Shader( Shader_Type type );

// Parameters (uniforms or attributes) ////////////////////////////////////////

typedef struct Shader_Param Shader_Param;

struct Shader_Param {

	GLchar*     name;

	GLuint      loc;
	Shader_Type type;
       
};

// Arguments (a realization of a parameter) ///////////////////////////////////

typedef struct Shader_Arg Shader_Arg;

struct Shader_Arg {

	Shader_Type type;
	pointer     binding;

	byte        value[];

};

Shader_Arg* bind_Shader_argv( region_p R, int argc, 
                              Shader_Param* params, 
                              pointer* bindings );
Shader_Arg* argi_Shader( Shader_Arg* argv, int I );
pointer    value_Shader_arg( Shader_Arg* arg );

// Programs ///////////////////////////////////////////////////////////////////

typedef struct Program Program;

struct Program {

	GLuint        id;
	const char*   name;

	int         n_shaders;
	Shader**      shaders;

	GLint       n_uniforms;
	Shader_Param* uniforms;

	GLint       n_attribs;
	Shader_Param* attribs;

	bool  built;
	char* log;

};

Program*        define_Program( const char* name, int n_shaders, ... );
Program*         build_Program( const char* name, int n_shaders, Shader* shaders[] );
void            delete_Program( Program* pgm );

Shader_Param*   attrib_Program( const Program*, const char* name );
uint           attribc_Program( const Program* );
Shader_Param*  attribv_Program( const Program* );
Shader_Param*  attribi_Program( const Program*, uint argi );

uint          uniformc_Program( const Program* );
Shader_Param* uniformv_Program( const Program* );
Shader_Param* uniformi_Program( const Program*, uint uniformi );

bool          validate_Program( Program* pgm );
void               use_Program( Program* pgm, Shader_Arg* uniforms );

#endif
