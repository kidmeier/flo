#ifndef __math_matrix_h__
#define __math_matrix_h__

#include <assert.h>
#include "math.vec.h"

typedef struct {
	
	float4 _1;
	float4 _2;
	float4 _3;
	float4 _4;
	
} mat44;

enum matrix_index_e {
	
	c11 = 0, c21 = 4, c31 = 8,  c41 = 12,
	c12 = 1, c22 = 5, c32 = 9,  c42 = 13,
	c13 = 2, c23 = 6, c33 = 10, c43 = 14,
	c14 = 3, c24 = 7, c34 = 11, c44 = 15,
	
	r11 = 0,  r21 = 1,  r31 = 2,  r41 = 3,
	r12 = 4,  r22 = 5,  r32 = 6,  r42 = 7,
	r13 = 8,  r23 = 9,  r33 = 10, r43 = 11,
	r14 = 12, r24 = 13, r34 = 14, r44 = 15,
	
};

static inline float colrow( const mat44* M, const int col, const int row ) {
	
	float* m = (float*)M;
	
	assert( col >= 1 && col <= 4 );
	assert( row >= 1 && row <= 4 );
	
	return m[ (col-1)*4 + (row-1) ];
	
}

static inline float rowcol( const mat44* M, const int row, const int col ) {
	
	float* m = (float*)M;
	
	assert( col >= 1 && col <= 4 );
	assert( row >= 1 && row <= 4 );
	
	return m[ (col-1)*4 + (row-1) ];
	
}

static inline mat44 madd( const mat44 a, const mat44 b ) {

	return (mat44) { 
		vadd(a._1, b._1),
		vadd(a._2, b._2),
		vadd(a._3, b._3),
		vadd(a._4, b._4)
	};

}

static inline mat44 msub( const mat44 a, const mat44 b ) {

	return (mat44) {
		vsub(a._1, b._1),
		vsub(a._2, b._2),
		vsub(a._3, b._3),
		vsub(a._4, b._4)
	};

}

static inline mat44 mscale( const float s, const mat44 m ) {
	
	return (mat44) {
		vscale(s, m._1),
		vscale(s, m._2),
		vscale(s, m._3),
		vscale(s, m._4)
	};

}

static inline float4 mtransform( const mat44 m, const float4 v ) {
  
	return (float4) {
		m._1.x*v.x + m._2.x*v.y + m._3.x*v.z + m._4.x*v.w,
		m._1.y*v.x + m._2.y*v.y + m._3.y*v.z + m._4.y*v.w,
		m._1.z*v.x + m._2.z*v.y + m._3.z*v.z + m._4.z*v.w,
		m._1.w*v.x + m._2.w*v.y + m._3.w*v.z + m._4.w*v.w
	};
	
}

static inline mat44 mmul( const mat44 m, const mat44 n ) {
  
  return (mat44) {
	  ._1 = { m._1.x*n._1.x + m._2.x*n._1.y + m._3.x*n._1.z + m._4.x*n._1.w,
	          m._1.y*n._1.x + m._2.y*n._1.y + m._3.y*n._1.z + m._4.y*n._1.w,
	          m._1.z*n._1.x + m._2.z*n._1.y + m._3.z*n._1.z + m._4.z*n._1.w,
	          m._1.w*n._1.x + m._2.w*n._1.y + m._3.w*n._1.z + m._4.w*n._1.w },
      
	  ._2 = { m._1.x*n._2.x + m._2.x*n._2.y + m._3.x*n._2.z + m._4.x*n._2.w,
	          m._1.y*n._2.x + m._2.y*n._2.y + m._3.y*n._2.z + m._4.y*n._2.w,
	          m._1.z*n._2.x + m._2.z*n._2.y + m._3.z*n._2.z + m._4.z*n._2.w,
	          m._1.w*n._2.x + m._2.w*n._2.y + m._3.w*n._2.z + m._4.w*n._2.w },
	
	  ._3 = { m._1.x*n._3.x + m._2.x*n._3.y + m._3.x*n._3.z + m._4.x*n._3.w,
	          m._1.y*n._3.x + m._2.y*n._3.y + m._3.y*n._3.z + m._4.y*n._3.w,
	          m._1.z*n._3.x + m._2.z*n._3.y + m._3.z*n._3.z + m._4.z*n._3.w,
	          m._1.w*n._3.x + m._2.w*n._3.y + m._3.w*n._3.z + m._4.w*n._3.w },
	  
	  ._4 = { m._1.x*n._4.x + m._2.x*n._4.y + m._3.x*n._4.z + m._4.x*n._4.w,
	          m._1.y*n._4.x + m._2.y*n._4.y + m._3.y*n._4.z + m._4.y*n._4.w,
	          m._1.z*n._4.x + m._2.z*n._4.y + m._3.z*n._4.z + m._4.z*n._4.w,
	          m._1.w*n._4.x + m._2.w*n._4.y + m._3.w*n._4.z + m._4.w*n._4.w }
  };
  
}

static inline mat44 mmulv( const mat44* m, const mat44* n ) {
  
  return (mat44) {
	  ._1 = { 
		 m->_1.x*n->_1.x + m->_2.x*n->_1.y + m->_3.x*n->_1.z + m->_4.x*n->_1.w,
		 m->_1.y*n->_1.x + m->_2.y*n->_1.y + m->_3.y*n->_1.z + m->_4.y*n->_1.w,
		 m->_1.z*n->_1.x + m->_2.z*n->_1.y + m->_3.z*n->_1.z + m->_4.z*n->_1.w,
		 m->_1.w*n->_1.x + m->_2.w*n->_1.y + m->_3.w*n->_1.z + m->_4.w*n->_1.w
	  },
      
	  ._2 = { 
		 m->_1.x*n->_2.x + m->_2.x*n->_2.y + m->_3.x*n->_2.z + m->_4.x*n->_2.w,
		 m->_1.y*n->_2.x + m->_2.y*n->_2.y + m->_3.y*n->_2.z + m->_4.y*n->_2.w,
		 m->_1.z*n->_2.x + m->_2.z*n->_2.y + m->_3.z*n->_2.z + m->_4.z*n->_2.w,
		 m->_1.w*n->_2.x + m->_2.w*n->_2.y + m->_3.w*n->_2.z + m->_4.w*n->_2.w
	  },
		   
	  ._3 = { 
		 m->_1.x*n->_3.x + m->_2.x*n->_3.y + m->_3.x*n->_3.z + m->_4.x*n->_3.w,
		 m->_1.y*n->_3.x + m->_2.y*n->_3.y + m->_3.y*n->_3.z + m->_4.y*n->_3.w,
		 m->_1.z*n->_3.x + m->_2.z*n->_3.y + m->_3.z*n->_3.z + m->_4.z*n->_3.w,
		 m->_1.w*n->_3.x + m->_2.w*n->_3.y + m->_3.w*n->_3.z + m->_4.w*n->_3.w
	  },
		   
	  ._4 = { 
		 m->_1.x*n->_4.x + m->_2.x*n->_4.y + m->_3.x*n->_4.z + m->_4.x*n->_4.w,
		 m->_1.y*n->_4.x + m->_2.y*n->_4.y + m->_3.y*n->_4.z + m->_4.y*n->_4.w,
		 m->_1.z*n->_4.x + m->_2.z*n->_4.y + m->_3.z*n->_4.z + m->_4.z*n->_4.w,
		 m->_1.w*n->_4.x + m->_2.w*n->_4.y + m->_3.w*n->_4.z + m->_4.w*n->_4.w
	  }
  };
  
}

static inline mat44 mtranspose( const mat44 m ) {

	return (mat44) {
		._1 = { m._1.x, m._2.x, m._3.x, m._4.x },
		._2 = { m._1.y, m._2.y, m._3.y, m._4.y },
		._3 = { m._1.z, m._2.z, m._3.z, m._4.z },
		._4 = { m._1.w, m._2.w, m._3.w, m._4.w }
	};

}

// Take the determinant of the upper-left 3x3 matrix
static inline float mdet33( const mat44 m ) {

	return 		  
		  m._1.x * (m._2.y*m._3.z - m._3.y*m._2.z)
		+ m._2.x * (m._3.y*m._1.z - m._1.y*m._3.z)
		+ m._3.x * (m._1.y*m._2.z - m._2.y*m._1.z);
	
}

// Take the inverse of the upper-left 3x3 matrix
static inline mat44 minverse33( const mat44 m ) {

	float det = mdet33( m );

	assert( 0.f != det );
	
	float oodet = 1.f / det;

	return (mat44) {
		._1 = {
			 oodet * (m._2.y*m._3.z - m._2.z*m._3.y),
			-oodet * (m._1.y*m._3.z - m._1.z*m._3.y),
			 oodet * (m._1.y*m._2.z - m._1.z*m._2.y),
			 0.f
		},

		._2 = {
			-oodet * (m._2.x*m._3.z - m._2.z*m._3.x),		
			 oodet * (m._1.x*m._3.z - m._1.z*m._3.x),
			-oodet * (m._1.x*m._2.z - m._1.z*m._2.x),
			 0.f	
		},

		._3 = {
			 oodet * (m._2.x*m._3.y - m._2.y*m._3.x),
			-oodet * (m._1.x*m._3.y - m._1.y*m._3.x),
			 oodet * (m._1.x*m._2.y - m._1.y*m._2.x),
			 0.f
		},

		._4 = { 0.f, 0.f, 0.f, 1.f }
		
	};
			
}

// Transformations
static inline mat44 mtranslation( const float4 v ) {
	
	return (mat44) {
		._1 = { 1.f, 0.f, 0.f, 0.f },
		._2 = { 0.f, 1.f, 0.f, 0.f },
		._3 = { 0.f, 0.f, 1.f, 0.f },
		._4 = { v.x, v.y, v.z, v.w }
	};

}

static inline mat44 mscaling( const float4 v ) {
	
	float sx = v.x / v.w;
	float sy = v.y / v.w;
	float sz = v.z / v.w;
	
	return (mat44) {
		._1 = {  sx, 0.f, 0.f, 0.f },
		._2 = { 0.f,  sy, 0.f, 0.f },
		._3 = { 0.f, 0.f,  sz, 0.f },
		._4 = { 0.f, 0.f, 0.f, 1.f }
	};

}

static inline mat44 mrotation( float theta, float x, float y, float z ) {
	
	float c = cos(theta);
	float s = sin(theta);
	float t = 1.0f - c;
	
	// Real-time Rendering, pg. 71
	return (mat44) {
		._1 = {   t*x*x + c, t*x*y + s*z, t*x*z - s*y, 0.f },
		._2 = { t*x*y - s*z,   t*y*y + c, t*y*z + s*x, 0.f },
		._3 = { t*x*z + s*y, t*y*z - s*x,   t*z*z + c, 0.f },
		._4 = {         0.f,         0.f,         0.f, 1.f }
	};
	
}

static inline mat44 mXrotation( float theta ) {

  return mrotation( theta, 1.f, 0.f, 0.f );

}

static inline mat44 mYrotation( float theta ) {

  return mrotation( theta, 0.f, 1.f, 0.f );

}

static inline mat44 mZrotation( float theta ) {

  return mrotation( theta, 0.f, 0.f, 1.f );

}

static inline mat44 mfrustum( float l, float r, float b, float t, float n, float f ) {

	// Real-time Rendering, pg. 95
	return (mat44){
		._1 = {  (2.f*n) / (r - l),                0.f,                  0.f,  0.f },
		._2 = {               0.0f,  (2.f*n) / (t - b),                  0.f,  0.f },
		._3 = {  (r + l) / (r - l),  (t + b) / (t - b),   -(f + n) / (f - n), -1.f },
		._4 = {                0.f,                0.f, -(2.f*f*n) / (f - n),  0.f }
	};

}

static inline mat44 mortho(float l, float r, float b, float t, float n, float f) {
  
	// Real-time Rendering, pg. 91
	return (mat44) {
		._1 = {      2.f / (r - l),               0.f,                 0.f, 0.f },
		._2 = {                0.f,     2.f / (t - b),                 0.f, 0.f },
		._3 = {                0.f,               0.f,       2.f / (f - n), 0.f },
		._4 = { -(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1.f }
	};
	
}

// Derive the rotation matrix corresponding to the given quaternion
static inline mat44 qmatrix( const float4 q ) {

	float xx = q.x*q.x;
	float yy = q.y*q.y;
	float zz = q.z*q.z;

	// Real-time Rendering, pg. 76
	return (mat44) {
		._1 = {     1.f - 2.f*yy - 2.f*zz, 2.f*q.x*q.y + 2.f*q.z*q.w, 2.f*q.x*q.z - 2.f*q.y*q.w, 0.f },
		._2 = { 2.f*q.x*q.y - 2.f*q.z*q.w,     1.f - 2.f*xx - 2.f*zz, 2.f*q.y*q.z + 2.f*q.x*q.w, 0.f },
		._3 = { 2.f*q.x*q.z + 2.f*q.y*q.w, 2.f*q.y*q.z - 2.f*q.x*q.w,     1.f - 2.f*xx - 2.f*yy, 0.f },
		._4 = {                       0.f,                        0.f,                      0.f, 1.f }
	};

}

// Constants
extern const mat44 identity_MAT44;
extern const mat44 zero_MAT44;

// Printf 
void init_printf_MAT44(void);

#endif
