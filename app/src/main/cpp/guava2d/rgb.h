#ifndef RGB_H_
#define RGB_H_

namespace g2d {

struct rgb
{
	rgb()
	: r(0), g(0), b(0)
	{ }

	rgb(float r, float g, float b)
	: r(r), g(g), b(b)
	{ }

	rgb
	operator-(const rgb& o) const
	{ return rgb(r - o.r, g - o.g, b - o.b); }

	rgb
	operator+(const rgb& o) const
	{ return rgb(r + o.r, g + o.g, b + o.b); }

	rgb
	operator*(float s) const
	{ return rgb(r*s, g*s, b*s); }

	float r, g, b;
};

struct rgba
{
	rgba()
	: r(0), g(0), b(0), a(0)
	{ }

	rgba(float r, float g, float b, float a)
	: r(r), g(g), b(b), a(a)
	{ }

	rgba(const rgb& color, float a)
	: r(color.r), g(color.g), b(color.b), a(a)
	{ }

	rgba
	operator-(const rgba& o) const
	{ return rgba(r - o.r, g - o.g, b - o.b, a - o.a); }

	rgba
	operator+(const rgba& o) const
	{ return rgba(r + o.r, g + o.g, b + o.b, a + o.a); }

	rgba
	operator*(float s) const
	{ return rgba(r*s, g*s, b*s, a*s); }

	float r, g, b, a;
};

inline rgb
operator*(float s, const rgb& o)
{
	return rgb(s*o.r, s*o.g, s*o.b);
}

inline rgba
operator*(float s, const rgba& o)
{
	return rgba(s*o.r, s*o.g, s*o.b, s*o.a);
}

}

#endif // RGB_H_
