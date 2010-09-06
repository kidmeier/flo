#ifndef __sys_dll_h__
#define __sys_dll_h__

#include "core.features.h"

#define dllSelf "self"

#if defined(feature_POSIX) 

#include <dlfcn.h>
#include <string.h>

static inline
void* open_DLL( const char* name ) {

	if( 0 == strcmp( name, dllSelf ) )
		return dlopen(NULL, RTLD_LAZY);
	else
		return dlopen(name, RTLD_LAZY);

}

static inline
void* lookup_DLL( void* dll, const char* sym ) {

	return dlsym( dll, sym );

}

static inline
int close_DLL( void* dll ) {

	return dlclose(dll);

}

#else
#error Unsupported platform
#endif

#endif
