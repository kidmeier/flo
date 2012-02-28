#ifndef __res_core_h__
#define __res_core_h__

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core.types.h"
#include "math.matrix.h"
#include "math.vec.h"

typedef struct Resource Resource;
typedef struct Res_Type Res_Type;

typedef Resource *(*import_Resource_f)( const char *name, size_t szbuf, const pointer buf );
typedef void      (*write_Resource_f)( pointer res, FILE *outp );
typedef pointer  *(  *read_Resource_f)( FILE *inp );

struct Res_Type {
	
	char              id[4];

	write_Resource_f write;
	read_Resource_f   read;

	Res_Type         *next;

};

struct Resource {

	char     *name;

	pointer   data;

	unsigned  refc;

	Res_Type *type;

	Resource *parent;
	Resource *child;

	Resource *next;

};

void    register_Res_importer( const char id[4],
                               const char *ext,
                               import_Resource_f importfunc );
void    register_Res_type( const char id[4],
                           write_Resource_f writefunc,
                           read_Resource_f readfunc );

void         add_Res_path( const char *scheme, const char *path );

Resource *import_Res( const char *name, const char *path );
Resource    *new_Res( Resource *parent,
                      const char *name,
                      const char typeid[4],
                      pointer data );
size_t     write_Res( Resource *res, const char *outdir );
Resource   *read_Res( const char *path );

#endif
