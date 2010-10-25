#ifndef __r_md5_h__
#define __r_md5_h__

#include "gl.shader.h"
#include "mm.region.h"
#include "r.drawable.h"
#include "res.md5.h"

Drawable* drawable_MD5( region_p R, 
                        Program* pgm, 
                        md5model_p mdl, 
                        int which_mesh );

#endif
