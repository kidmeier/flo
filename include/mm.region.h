#ifndef __mm_region_h__
#define __mm_region_h__

typedef struct region_s region_t;
typedef region_t* region_p;

#include "core.types.h"
#include "mm.zone.h"

// Memory regions, loosely based on (citation).
//
// A region is a memory allocation area that supports fast allocation and 
// aggregate deallocation. That is, freeing a region frees _all_ memory 
// allocated from that region, no matter how many different allocations
// occured in that region. Also known as "poor man's garbage collection".
//
// Notes: Operations on a region_p are not thread-safe; the caller is
//        responsible for ensuring each region_p is accessed sequentially

// Allocate a new region with given `name`. The name has no significance other
// than auditing/reporting/debugging.
//
// @Z    - zone_p from which to allocate the region
// @name - C-string naming the region
region_p region( zone_p Z, const char* name );

// Allocate `sz` bytes from region `R`
//
// @R  - region from which to allocate bytes.
// @sz - size in bytes to allocate; must be less than region_MM_pagesize
pointer  ralloc( region_p R, uint16 sz );

// Free all pages allocated by `R`.
//
// @R - region to collect
void     rcollect( region_p R );

// Free region `R` and all memory allocated from `R`.
//
// @R - region to free
void     rfree( region_p R );

// Initialize the region system. Must be called before any other region-related
// functions are called.
int      region_MM_init( zone_p zone );
void     region_MM_shutdown( void );

#endif
