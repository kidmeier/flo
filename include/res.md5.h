#ifndef __res_md5_h__
#define __res_md5_h__

#include "math.vec.h"
#include "res.core.h"
#include "sys.dll.h"

// Resource loader
dllExport Resource *import_MD5( const char *name, size_t sz, const pointer data );

#endif
