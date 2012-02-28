#ifndef __sys_file_H__
#define __sys_file_H__

#include "core.features.h"
#include "core.types.h"

#if defined( feature_POSIX )

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define fileSeparator '/'
#define fileSeparator_string "/"

#elif defined( feature_WIN32 )

#include <windows.h>

#define fileSeparator '\\'
#define fileSeparator_string "\\"

#else
#error "Unsupported platform"
#endif

bool Fs_exists( const char *path );
int  Fs_mkdirs( const char *path );

#endif
