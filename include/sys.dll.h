#ifndef __sys_dll_h__
#define __sys_dll_h__

#include "core.features.h"

#define dllSelf "self"

#if defined(feature_POSIX) 

#include <dlfcn.h>
#include <string.h>

typedef void* dll_t;

#define dllExport

static inline
dll_t open_DLL( const char *name ) {

	if( 0 == strcmp( name, dllSelf ) )
		return dlopen(NULL, RTLD_LAZY);
	else
		return dlopen(name, RTLD_LAZY);

}

static inline
void* lookup_DLL( dll_t dll, const char* sym ) {

	return dlsym( dll, sym );

}

static inline
int close_DLL( dll_t dll ) {

	return dlclose(dll);

}

#elif defined( feature_WIN32 )

#include <windows.h>
typedef HMODULE dll_t;

#define dllExport __declspec(dllexport)

static inline
void* open_DLL( const char *name ) {

	if( 0 == strcmp( name, dllSelf ) )
		return GetModuleHandle( NULL );
	else
		return LoadLibrary( name );

}

static inline
void* lookup_DLL( dll_t dll, const char* sym ) {

	return GetProcAddress( dll, sym );

}

static inline
int close_DLL( dll_t dll ) {

	return !FreeLibrary( dll );

}

#else
#error Unsupported platform
#endif

#endif
