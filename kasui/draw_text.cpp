#include <guava2d/font.h>
#include <guava2d/vec2.h>

#include "render.h"

void draw_text(const g2d::font *font, const g2d::vec2& pos, const wchar_t *str)
{
	float x = pos.x;

	const auto texture = font->get_texture();

	for (const wchar_t *p = str; *p; ++p) {
		const auto g = font->find_glyph(*p);

		const auto& t0 = g->texuv[0];
		const auto& t1 = g->texuv[1];
		const auto& t2 = g->texuv[2];
		const auto& t3 = g->texuv[3];

		const float x0 = x + g->left;
		const float x1 = x0 + g->width;
		const float y0 = pos.y + g->top;
		const float y1 = y0 - g->height;

		render::draw_quad(
			texture,
			{ { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } },
			{ { t0.x, t0.y }, { t1.x, t1.y }, { t2.x, t2.y }, { t3.x, t3.y } },
			50);

		x += g->advance_x;
	}
}
