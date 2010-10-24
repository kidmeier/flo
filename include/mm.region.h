#ifndef __mm_region_h__
#define __mm_region_h__

typedef struct region_s region_t;
typedef region_t* region_p;

#include "core.types.h"

// Memory regions, loosely based on (citation).
//
// A region is a memory allocation area that supports fast allocation and 
// aggregate deallocation. That is, freeing a region frees _all_ memory 
// allocated from that region, no matter how many different allocations
// occured in that region. Also known as "poor man's garbage collection".
//
// There are two ways of requesting memory from a region. The first, `ralloc`
// allocates an arbitrarily sized chunk of memory which is not freed until
// the whole region is collected with `rcollect`.
//
// The other mode is a freestore of equally sized pages of memory. Individual
// pages can be allocated and freed without having to free all memory allocated
// from the region.
//
// Notes: Operations on a region_p are not thread-safe; the caller is
//        responsible for ensuring each region_p is accessed sequentially.
//        It is probably best to keep regions thread-local.

// Allocate a new region with given `name`. The name has no significance other
// than auditing/reporting/debugging.
//
// @name - C-string naming the region
region_p region( const char* name );

// Allocate `sz` bytes from region `R`
//
// @R  - region from which to allocate bytes.
// @sz - size in bytes to allocate; must be less than region_MM_pagesize
pointer  ralloc( region_p R, uint16 sz );

// Allocate a page from region `R`.
//
// @R - region from which to allocate a page
pointer  rallocpg( region_p R );

// Return a page of storage back to region `R`
//
// @R - the region to return the page
// @p - pointer to the beginning of the page
void     rfreepg( region_p R, const pointer pg );

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
int      region_MM_init( void );
void     region_MM_shutdown( void );

#endif
