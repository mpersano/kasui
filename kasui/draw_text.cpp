#include <guava2d/font.h>
#include <guava2d/vec2.h>

#include "render.h"
#include "draw_text.h"

void draw_text(const g2d::font *font, const g2d::vec2& pos, const wchar_t *str)
{
	float x = pos.x;

	const auto texture = font->get_texture();

	for (const wchar_t *p = str; *p; ++p) {
		const auto g = font->find_glyph(*p);

		const float x0 = x + g->left;
		const float x1 = x0 + g->width;
		const float y0 = pos.y + g->top;
		const float y1 = y0 - g->height;

		render::draw_quad(
			texture,
			{ { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } },
			{ g->texuv[0], g->texuv[1], g->texuv[2], g->texuv[3] },
			50);

		x += g->advance_x;
	}
}

void draw_text(const g2d::font *font, const g2d::vec2& pos, text_align align, const wchar_t *str)
{
	switch (align) {
		case text_align::RIGHT:
			draw_text(font, { pos.x - font->get_string_width(str), pos.y }, str);
			break;

		case text_align::CENTER:
			draw_text(font, { pos.x - .5f*font->get_string_width(str), pos.y }, str);
			break;

		default:
			draw_text(font, pos, str);
	}
}
