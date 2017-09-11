#ifndef VEC3_H_
#define VEC3_H_

#include <math.h>

namespace g2d {

struct vec3
{
	vec3(float x = 0, float y = 0, float z = 0)
	: x(x), y(y), z(z)
	{ }

	void set(float x_, float y_, float z_)
	{ x = x_; y = y_; z = z_; }

	void zero()
	{ x = y = z = 0.f; }

	float dot_product(const vec3& v) const
	{ return x*v.x + y*v.y + z*v.z; }

	float length_squared() const
	{ return dot_product(*this); }

	float length() const
	{ return sqrtf(length_squared()); }

	vec3& normalize()
	{ *this *= 1.f/length(); return *this; }

	vec3
	cross_product(const vec3& v) const
	{ return vec3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }

	vec3& operator+=(const vec3& v)
	{ x += v.x; y += v.y; z += v.z; return *this; }

	vec3 operator+(const vec3& v) const
	{ return vec3(x + v.x, y + v.y, z + v.z); } 

	vec3& operator-=(const vec3& v)
	{ x -= v.x; y -= v.y; z -= v.z; return *this; }

	vec3 operator-(const vec3& v) const
	{ return vec3(x - v.x, y - v.y, z - v.z); }

	vec3& operator*=(float s)
	{ x *= s; y *= s; z *= s; return *this; }

	vec3 operator*(float s) const
	{ return vec3(x*s, y*s, z*s); }

	float
	distance_squared(const vec3& v)
	{ return (v - *this).length_squared(); }

	float
	distance(const vec3& v)
	{ return sqrtf(distance_squared(v)); }

	float x, y, z;
};

inline vec3
operator*(float s, const vec3& v)
{
	return vec3(s*v.x, s*v.y, s*v.z);
}

struct mat4
{
	mat4(float m11 = 0, float m12 = 0, float m13 = 0, float m14 = 0,
		float m21 = 0, float m22 = 0, float m23 = 0, float m24 = 0,
		float m31 = 0, float m32 = 0, float m33 = 0, float m34 = 0)
	: m11(m11), m12(m12), m13(m13), m14(m14)
	, m21(m21), m22(m22), m23(m23), m24(m24)
	, m31(m31), m32(m32), m33(m33), m34(m34)
	{ }

	static mat4 identity()
	{
		return mat4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0);
	}
	
	static mat4 rotation_around_x(const float ang)
	{
		float c = cosf(ang);
		float s = sinf(ang);
	
		return mat4(
			1,  0,  0,  0,
			0,  c, -s,  0,
			0,  s,  c,  0);
	}
	
	static mat4 rotation_around_y(const float ang)
	{
		float c = cosf(ang);
		float s = sinf(ang);
	
		return mat4(
			c,  0, -s,  0,
			0,  1,  0,  0,
			s,  0,  c,  0);
	}
	
	static mat4 rotation_around_z(const float ang)
	{
		float c = cosf(ang);
		float s = sinf(ang);
	
		return mat4(
			c, -s,  0,  0,
			s,  c,  0,  0,
			0,  0,  1,  0);
	}
	
	static mat4 rotation_from_axis_and_angle(const vec3& axis, const float phi)
	{
		float rcos = cosf(phi);
		float rsin = sinf(phi);
	
		const float u = axis.x;
		const float v = axis.y;
		const float w = axis.z;
	
		const float l11 = rcos + u*u*(1. - rcos);
		const float l12 = -w*rsin + u*v*(1. - rcos);
		const float l13 = v*rsin + u*w*(1. - rcos);
	
		const float l21 = w*rsin + v*u*(1.- rcos);
		const float l22 = rcos + v*v*(1. - rcos);
		const float l23 = -u*rsin + v*w*(1. - rcos);
	
		const float l31 = -v*rsin + w*u*(1. - rcos);
		const float l32 = u*rsin + w*v*(1. - rcos);
		const float l33 = rcos + w*w*(1. - rcos);
	
		return mat4(
			l11, l12, l13, 0,
			l21, l22, l23, 0,
			l31, l32, l33, 0);
	}
	
	static mat4 translation(const float x, const float y, const float z)
	{
		return mat4(
			1, 0, 0, x,
			0, 1, 0, y,
			0, 0, 1, z);
	}
	
	static mat4 translation(const vec3& pos)
	{ return translation(pos.x, pos.y, pos.z); }
	
	static mat4 scale(const float s)
	{
		return scale(s, s, s);
	}
	
	static mat4 scale(const float sx, const float sy, const float sz)
	{
		return mat4(
			sx, 0, 0, 0,
			0, sy, 0, 0,
			0, 0, sz, 0);
	}
	
	mat4 operator*(const mat4& m) const
	{
		const float l11 = m11*m.m11 + m12*m.m21 + m13*m.m31;
		const float l12 = m11*m.m12 + m12*m.m22 + m13*m.m32;
		const float l13 = m11*m.m13 + m12*m.m23 + m13*m.m33;
		const float l14 = m11*m.m14 + m12*m.m24 + m13*m.m34 + m14;
	
		const float l21 = m21*m.m11 + m22*m.m21 + m23*m.m31;
		const float l22 = m21*m.m12 + m22*m.m22 + m23*m.m32;
		const float l23 = m21*m.m13 + m22*m.m23 + m23*m.m33;
		const float l24 = m21*m.m14 + m22*m.m24 + m23*m.m34 + m24;
	
		const float l31 = m31*m.m11 + m32*m.m21 + m33*m.m31;
		const float l32 = m31*m.m12 + m32*m.m22 + m33*m.m32;
		const float l33 = m31*m.m13 + m32*m.m23 + m33*m.m33;
		const float l34 = m31*m.m14 + m32*m.m24 + m33*m.m34 + m34;
	
		return mat4(
			l11, l12, l13, l14,
			l21, l22, l23, l24,
			l31, l32, l33, l34);
	}
	
	mat4& operator*=(const mat4& m)
	{
		return *this = *this*m;
	}
	
	mat4 transpose() const
	{
		const float l11 = m11, l12 = m21, l13 = m31; 
		const float l21 = m12, l22 = m22, l23 = m32;
		const float l31 = m13, l32 = m23, l33 = m33;
	
		return mat4(
			l11, l12, l13, 0,
			l21, l22, l23, 0,
			l31, l32, l33, 0);
	}
	
	mat4 invert() const
	{
	        const float d = 1.f / (m11*(m22*m33 - m32*m23) - (m21*(m12*m33 - m32*m13) +
		  m31*(m22*m13 - m12*m23)));
	
	        const float l11 = d*(m22*m33 - m32*m23);
	        const float l21 = d*(m31*m23 - m21*m33);
	        const float l31 = d*(m21*m32 - m31*m22);
	
	        const float l12 = d*(m32*m13 - m12*m33);
	        const float l22 = d*(m11*m33 - m31*m13);
	        const float l32 = d*(m31*m12 - m11*m32);
	
	        const float l13 = d*(m12*m23 - m22*m13);
	        const float l23 = d*(m21*m13 - m11*m23);
	        const float l33 = d*(m11*m22 - m21*m12);
	
	        const float l14 =
			d*(m22*(m13*m34 - m33*m14) +
				m32*(m23*m14 - m13*m24) -
				m12*(m23*m34 - m33*m24));

	        const float l24 =
			d*(m11*(m23*m34 - m33*m24) +
				m21*(m33*m14 - m13*m34) +
				m31*(m13*m24 - m23*m14));

	        const float l34 =
			d*(m21*(m12*m34 - m32*m14) +
				m31*(m22*m14 - m12*m24) -
				m11*(m22*m34 - m32*m24));
	
		return mat4(
			l11, l12, l13, l14,
			l21, l22, l23, l24,
			l31, l32, l33, l34);
	}

	vec3 operator*(const vec3& v) const
	{
		const float x = m11*v.x + m12*v.y + m13*v.z + m14;
		const float y = m21*v.x + m22*v.y + m23*v.z + m24;
		const float z = m31*v.x + m32*v.y + m33*v.z + m34;
	
		return vec3(x, y, z);
	}
	
	vec3 rotate(const vec3& v) const
	{
		const float x = m11*v.x + m12*v.y + m13*v.z;
		const float y = m21*v.x + m22*v.y + m23*v.z;
		const float z = m31*v.x + m32*v.y + m33*v.z;
	
		return vec3(x, y, z);
	}

	static mat4 ortho(float left, float right, float bottom, float top, float close, float far)
	{
		const float a = 2./(right - left);
		const float b = 2./(top - bottom);
		const float c = -2./(far - close);
	
		const float tx = -(right + left)/(right - left);
		const float ty = -(top + bottom)/(top - bottom);
		const float tz = -(far + close)/(far - close);
	
		return mat4(
			a, 0, 0, tx,
			0, b, 0, ty,
			0, 0, c, tz);
	}

	float m11, m12, m13, m14;
	float m21, m22, m23, m24;
	float m31, m32, m33, m34;
};

}

#endif // VEC3_H_
