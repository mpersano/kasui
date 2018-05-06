#ifndef FONT_H_
#define FONT_H_

#include <stdarg.h>

#include "g2dgl.h"
#include "vec2.h"

namespace g2d {

class texture;
class program;

struct glyph_info
{
	int width, height;
	int left, top;
	int advance_x;
	vec2 texuv[4]; // texture coordinates
	const texture *texture_;
};

class font
{
public:
	font(const char *source);
	~font();

	const glyph_info *find_glyph(wchar_t ch) const
	{ return glyph_info_map_[ch]; }

	int get_string_width(const wchar_t *str, size_t len) const;
	int get_string_width(const wchar_t *str) const;

	const texture *get_texture() const
	{ return texture_; }

private:
	glyph_info *glyph_info_map_[1<<16];
	const texture *texture_;
};

}

#endif /* FONT_H_ */
