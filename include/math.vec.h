#ifndef __math_vec_h__
#define __math_vec_h__

#include <math.h>

typedef struct {

  float x;
  float y;
  float z;
  float w;

} float4;

// Vector opeators
static inline float4 vadd( float4 a, float4 b ) {

  return (float4){ a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w };

}

static inline float4 vsub( float4 a, float4 b) {

  return (float4){ a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w };

}

static inline float4 vmul( float4 a, float4 b) {

  return (float4){ a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w };

}

static inline float vdot( float4 a, float4 b) {

  return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;

}

static inline float4 vcross( float4 a, float4 b ) {

	return (float4) {
		a.y*b.z - a.z*b.y,
		a.z*b.x - a.x*b.z,
		a.x*b.y - a.y*b.x,
		// This is not really meaningful but may sometimes be functionally useful?
		a.w * b.w
	};

}

static inline float4 vscale( float s, float4 v ) {

  return (float4){ s*v.x, s*v.y, s*v.z, s*v.w };

}

static inline float4 vneg( float4 v ) {

  return (float4){ -v.x, -v.y, -v.z, -v.w };

}

static inline float vlength2( float4 v ) {

  return v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w;

}

static inline float vlength( float4 v ) {

  return sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);

}

static inline float4 vnormal( float4 v ) {

  float ool = 1.0f / vlength(v);
  return (float4){ ool*v.x, ool*v.y, ool*v.z, ool*v.w };

}

// Quaternion operators
#define qadd vadd
#define qsub vsub
#define qnorm vlength
#define qnorm2 vlength2
#define qversor vnormal
#define qscale vscale

static inline float4 qmul( float4 q, float4 r ) {

	return (float4) {
		q.w*r.x + q.x*r.w + q.y*r.z - q.z*r.y,
		q.w*r.y - q.x*r.z + q.y*r.w + q.z*r.x,
		q.w*r.z + q.x*r.y - q.y*r.x + q.z*r.w,
		q.w*r.w - q.x*r.x - q.y*r.y - q.z*r.z
	};

}

static inline float4 qconj( const float4 q ) {

  return (float4){ -q.x, -q.y, -q.z, q.w };

}

static inline float4 qrecip( const float4 q ) {

  return qscale( 1.0f/qnorm2(q), qconj(q) );

}

static inline float4 qrot( const float4 q, const float4 p ) {

	return qmul(qmul(q,p), qconj(q));

}

// Derive the quaternion corresponding to the passed in axis-angle
// From: http://www.flipcode.com/documents/matrfaq.html#Q56
static inline float4 qaxis( const float4 axis ) {

	float s = sin( axis.w / 2.f );
	float c = cos( axis.w / 2.f );

	return qversor((float4) { axis.x * s, axis.y * s, axis.z * s, c });

}

// Derive the axis-angle corresponding to the passed in quaternion
// From: http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation#Quaternion_to_axis-angle
// Note: This may blow up for very small rotations since the magnitude
//       of the quaternion will be related to the angle of rotation.
static inline float4 axisq( const float4 q ) {

	float scale = sqrt( q.x*q.x + q.y*q.y + q.z*q.z );
	return (float4) { q.x / scale, q.y / scale, q.z / scale, 2.f * acos(q.w) };

}

// Derive the quaternion resulting from the sequence of euler rotations
// From: http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation#Quaternion_from_Euler_angles
static inline float4 qeuler(float pitch, float yaw, float roll) {

	float sinp = sin( pitch / 2.f );
	float siny = sin( yaw / 2.f );
	float sinr = sin( roll / 2.f );
	float cosp = cos( pitch / 2.f );
	float cosy = cos( yaw / 2.f );
	float cosr = cos( roll / 2.f );
	
	return (float4) { 
		sinr*cosp*cosy - cosr*sinp*siny,
		cosr*sinp*cosy + sinr*cosp*siny,
		cosr*cosp*siny - sinr*sinp*cosy,
		cosr*cosp*cosy + sinr*sinp*siny
	};

}

// Derive the quaternion from the given spherical coordinates
// From: http://www.flipcode.com/documents/matrfaq.html#58
static inline float4 qspherical( float lat, float lon, float theta ) {

	float sina = sin( theta / 2.f );
	float cosa = cos( theta / 2.f );
	float sinla = sin( lat );
	float sinlo = sin( lon );
	float cosla = cos( lat );
	float coslo = cos( lon );

	return (float4) { sina*cosla*sinlo, sina*sinla, sina*sinla*coslo,	cosa };

}

// Constants
extern const float4 origin_PT;
extern const float4 zero_VEC;
extern const float4 xunit_VEC;
extern const float4 yunit_VEC;
extern const float4 zunit_VEC;
extern const float4 wunit_VEC;

// Printf
void init_printf_FLOAT4(void);

#endif
