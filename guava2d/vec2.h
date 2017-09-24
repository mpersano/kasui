#ifndef VEC2_H_
#define VEC2_H_

#include <cmath>

namespace g2d {

struct vec2
{
	vec2(float x = 0, float y = 0)
	: x(x), y(y)
	{ }

	vec2 operator*(float s) const
	{ return vec2(x*s, y*s); }

	vec2 operator*(const vec2& v) const
	{ return vec2(x*v.x, y*v.y); }

	vec2 operator/(float s) const
	{ return operator*(1./s); }

	vec2 operator/(const vec2& v) const
	{ return operator*(vec2(1./v.x, 1./v.y)); }

	vec2& operator*=(float s)
	{ x *= s; y *= s; return *this; }

	vec2 operator+(const vec2& v) const
	{ return vec2(x + v.x, y + v.y); }

	vec2& operator+=(const vec2& v)
	{ x += v.x; y += v.y; return *this; }

	vec2 operator-(const vec2& v) const
	{ return vec2(x - v.x, y - v.y); }

	vec2& operator-=(const vec2& v)
	{ x -= v.x; y -= v.y; return *this; }

	float dot(const vec2& v) const
	{ return x*v.x + y*v.y; }

	float length_squared() const
	{ return x*x + y*y; }

	float length() const
	{ return sqrt(length_squared()); }

	vec2& normalize()
	{ *this *= 1./length(); return *this; }

	vec2& set_length(float l)
	{ *this *= l/length(); return *this; }

	vec2 rotate(float ang) const
	{
		const float c = cos(ang);
		const float s = sin(ang);
		return vec2(x*c - y*s, y*c + x*s);
	}

	float distance(const vec2& v) const
	{
		vec2 d = v - *this;
		return d.length();
	}

	float x, y;
};

inline vec2
operator*(float s, const vec2& v)
{
	return vec2(s*v.x, s*v.y);
}

struct mat3
{
	mat3(float m00 = 0, float m01 = 0, float m02 = 0,
	     float m10 = 0, float m11 = 0, float m12 = 0)
	: m00(m00), m01(m01), m02(m02)
	, m10(m10), m11(m11), m12(m12)
	{ }

	static mat3 identity()
	{
		return mat3(
			1., 0., 0.,
			0., 1., 0.);
	}

	static mat3 translation(float x, float y)
	{
		return mat3(
			1, 0, x,
			0, 1, y);
	}

	static mat3 translation(const vec2& pos)
	{ return translation(pos.x, pos.y); }

	static mat3 scale(float s)
	{ return scale(s, s); }

	static mat3 scale(float sx, float sy)
	{
		return mat3(
			sx, 0., 0.,
			0., sy, 0.);
	}

	static mat3 rotation(float a)
	{
		const float c = cosf(a);
		const float s = sinf(a);

		return mat3(
			c, -s, 0.,
			s, c, 0.);
	}

	mat3& operator*=(const mat3& m)
	{
		return *this = *this*m;
	}

	mat3 operator*(const mat3& m) const
	{
		const float l00 = m00*m.m00 + m01*m.m10;
		const float l01 = m00*m.m01 + m01*m.m11;
		const float l02 = m00*m.m02 + m01*m.m12 + m02;

		const float l10 = m10*m.m00 + m11*m.m10;
		const float l11 = m10*m.m01 + m11*m.m11;
		const float l12 = m10*m.m02 + m11*m.m12 + m12;

		return mat3(
			l00, l01, l02,
			l10, l11, l12);
	}

	vec2 operator*(const vec2& v) const
	{
		const float x = m00*v.x + m01*v.y + m02;
		const float y = m10*v.x + m11*v.y + m12;
		return vec2(x, y);
	}

	float m00, m01, m02;
	float m10, m11, m12;
};

}

#endif // VEC2_H_
