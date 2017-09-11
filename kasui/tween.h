#ifndef TWEEN_H_
#define TWEEN_H_

#include <cmath>

template <class T>
struct tween {
	typedef T type;
};

template <class T>
struct linear_tween : tween<T> {
	T operator()(const T& a, const T& b, float t) const
	{
		return a + t*(b - a);
	}
};

template <class T>
struct quadratic_tween : tween<T> {
	T operator()(const T& a, const T& b, float t) const
	{
		return a + t*t*(b - a);
	}
};

template <class T>
struct in_cos_tween : tween<T> {
	T operator()(const T& a, const T& b, float t) const
	{
		const float f = cos((1 - t)*.5*M_PI);
		return a + f*(b - a);
	}
};

template <class T>
struct out_cos_tween : tween<T> {
	T operator()(const T& a, const T& b, float t) const
	{
		const float f = 1 - cos(t*.5*M_PI);
		return a + f*(b - a);
	}
};


// stolen from robert penner

template <class T>
struct in_back_tween : tween<T> {
	T operator()(const T& a, const T& b, float t) const
	{
		const float s = 1.70158;
		return a + t*t*((s + 1)*t - s)*(b - a);
	}
};

template <class T>
struct out_bounce_tween : tween<T> {
	T operator()(const T& a, const T& b, float t) const
	{
		float f;

		if (t < 1./2.75) {
			f = 7.5625*t*t;
		} else if (t < 2./2.75) {
			t -= 1.5/2.75;
			f = 7.5625*t*t + .75;
		} else if (t < 2.5/2.75) {
			t -= 2.25/2.75;
			f = 7.5625*t*t + .9375;
		} else {
			t -= 2.625/2.75;
			f = 7.5625*t*t + .984375;
		}

		return a + f*(b - a);
	}
};

#endif // TWEEN_H_
