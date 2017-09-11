#include <cstdio>
#include <cstring>
#include <cassert>
#include <cerrno>

#include <stdarg.h>
#include <ctype.h>
#include <wchar.h>

#include "panic.h"
#include "xwchar.h"
#include "texture.h"
#include "texture_manager.h"
#include "font.h"
#include "file.h"
#include "panic.h"

namespace g2d {

font::font(const char *source)
{
	::memset(glyph_info_map_, 0, sizeof glyph_info_map_);

	char path[512];
	
	sprintf(path, "%s.spr", source);
	file_input_stream file(path);

	int num_glyphs = file.read_uint16();

	// read textures

	sprintf(path, "%s.png", source);
	texture_ = texture_manager::get_instance().load(path);

	// read glyphs

	for (int i = 0; i < num_glyphs; i++) {
		wchar_t code = file.read_uint16();

		int left = static_cast<int8_t>(file.read_uint8());
		int top = static_cast<int8_t>(file.read_uint8());
		int advance_x = static_cast<int8_t>(file.read_uint8());

		const int u = file.read_uint16();
		const int v = file.read_uint16();
		const int w = file.read_uint16();
		const int h = file.read_uint16();

		glyph_info *g = new glyph_info;

		g->width = w;
		g->height = h;
		g->left = left;
		g->top = top;
		g->advance_x = advance_x;
		g->texture_ = texture_;

		const int texture_width = texture_->get_texture_width();
		const int texture_height = texture_->get_texture_height();

		int s = 2; // texture_->get_downsample_scale();

		const float u0 = static_cast<float>(u)/s/texture_width;
		const float v0 = static_cast<float>(v)/s/texture_height;

		const float du = static_cast<float>(w)/s/texture_width;
		const float dv = static_cast<float>(h)/s/texture_height;

		g->texuv[0] = vec2(u0, v0);
		g->texuv[1] = vec2(u0 + du, v0);
		g->texuv[2] = vec2(u0 + du, v0 + dv);
		g->texuv[3] = vec2(u0, v0 + dv);

		glyph_info_map_[code] = g;
	}
}

int
font::get_string_width(const wchar_t *str) const
{
	return get_string_width(str, xwcslen(str));
}

int
font::get_string_width(const wchar_t *str, size_t len) const
{
	int width = 0;

	for (const wchar_t *p = str; p != &str[len]; p++)
		width += find_glyph(*p)->advance_x;

	return width;
}

}
