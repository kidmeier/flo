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

	shBool            = GL_BOOL,
	shInt             = GL_INT,
	shFloat           = GL_FLOAT,
	shDouble          = GL_DOUBLE,
	shSampler1d       = GL_SAMPLER_1D,
	shSampler2d       = GL_SAMPLER_2D,
	shSampler3d       = GL_SAMPLER_3D,
	shSamplerCube     = GL_SAMPLER_CUBE,
	shSampler1dShadow = GL_SAMPLER_1D_SHADOW,
	shSampler2dShadow = GL_SAMPLER_2D_SHADOW

} shaderPrimitive_e;

typedef struct Shader_Type Shader_Type;

struct Shader_Type {

	GLenum              glType;

	shaderPrimitive_e   prim;
	uint8               primSize;

	struct {
		uint8 cols;
		uint8 rows;
	}                   shape;

	uint8               count;

};

int sizeof_Shade_Type( Shader_Type type );

// Parameters (uniforms or attributes) ////////////////////////////////////////

typedef struct Shader_Param Shader_Param;

struct Shader_Param {

	GLchar*     name;
	GLint       loc;
	Shader_Type type;
       
};

// Arguments (a realization of a parameter) ///////////////////////////////////

typedef struct Shader_Arg Shader_Arg;

struct Shader_Arg {

	GLint       loc;
	Shader_Type type;
	pointer     var;

	byte        value[];

};

Shader_Arg *alloc_Shader_argv( region_p R, int argc, Shader_Param *params );
Shader_Arg  *bind_Shader_argv( int argc, Shader_Arg *argv, ... );
Shader_Arg  *bind_Shader_args( int argc, Shader_Arg *argv, pointer *bindings );

Shader_Arg   *set_Shader_Arg( Shader_Arg *arg, pointer value );
Shader_Arg  *bind_Shader_Arg( Shader_Arg *arg, pointer binding );

Shader_Arg  *find_Shader_Arg( int argc, Shader_Param *params, Shader_Arg *argv, const char *name );

Shader_Arg  *next_Shader_Arg( Shader_Arg *arg );
Shader_Arg   *nth_Shader_Arg( Shader_Arg *argv, int N );

//pointer   binding_Shader_Arg( Shader_Arg *arg );
pointer     value_Shader_Arg( Shader_Arg *arg );

// Programs ///////////////////////////////////////////////////////////////////

typedef struct Program Program;

struct Program {

	GLuint        id;
	const char   *name;

	int         n_shaders;
	Shader      **shaders;

	GLint       n_uniforms;
	Shader_Param *uniforms;

	GLint       n_attribs;
	Shader_Param *attribs;

	bool  built;
	char *log;

};

// This is a variadic version of build_Program
Program*        define_Program( const char* name, 
                                int n_shaders, ... 
                            /*, int n_attribs, ... */
                            /*, int n_uniforms, ... */ );
Program*         build_Program( const char* name, 
                                int n_shaders, 
                                Shader* shaders[], 
                                int n_attribs, 
                                const char* attribs[],
                                int n_uniforms,
                                const char* uniforms[] );
void            delete_Program( Program* pgm );

Shader_Param*   attrib_Program( const Program*, const char* name );
uint           attribc_Program( const Program* );
Shader_Param*  attribv_Program( const Program* );
Shader_Param*  attribi_Program( const Program*, uint argi );

uint          uniformc_Program( const Program* );
Shader_Param* uniformv_Program( const Program* );
Shader_Param* uniformi_Program( const Program*, uint uniformi );

bool          validate_Program( Program* pgm );
void              load_Program_uniforms( Shader_Arg* argv );
void               use_Program( Program* pgm, Shader_Arg* uniforms );

#endif
