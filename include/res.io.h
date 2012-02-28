#ifndef __res_io_H__
#define __res_io_H__

#include <stdio.h>

#include "core.types.h"
#include "math.matrix.h"
#include "math.vec.h"

static inline size_t write_Res_buf( FILE *fp, size_t sz, const pointer buf ) {
	return fwrite( buf, 1, sz, fp );
}

static inline size_t write_Res_string( FILE *fp, const char *s ) {
	size_t len = strlen(s); 
	return sizeof(len) * fwrite( &len, sizeof(len), 1, fp ) + fwrite( s, 1, len, fp );
}

static inline size_t write_Res_int8  ( FILE *fp, int8_t   value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_int16 ( FILE *fp, int16_t  value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_int32 ( FILE *fp, int32_t  value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_int64 ( FILE *fp, int64_t  value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_uint8 ( FILE *fp, uint8_t  value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_uint16( FILE *fp, uint16_t value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_uint32( FILE *fp, uint32_t value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_uint64( FILE *fp, uint64_t value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_float ( FILE *fp, float    value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_float4( FILE *fp, float4   value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_double( FILE *fp, double   value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t write_Res_mat44 ( FILE *fp, mat44    value ) {
	return sizeof(value) * fwrite( &value, sizeof(value), 1, fp );
}

static inline size_t read_Res_buf   ( FILE *fp, size_t sz, const pointer s ) {
	return fread( s, 1, sz, fp );
}

static inline size_t read_Res_int8  ( FILE *fp, int8_t   *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_int16 ( FILE *fp, int16_t  *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_int32 ( FILE *fp, int32_t  *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_int64 ( FILE *fp, int64_t  *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_uint8 ( FILE *fp, uint8_t  *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_uint16( FILE *fp, uint16_t *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_uint32( FILE *fp, uint32_t *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_uint64( FILE *fp, uint64_t *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_float ( FILE *fp, float    *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_float4( FILE *fp, float4   *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_double( FILE *fp, double   *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

static inline size_t read_Res_mat44 ( FILE *fp, mat44    *value ) {
	return sizeof(*value) * fread( value, sizeof(*value), 1, fp );
}

#endif
