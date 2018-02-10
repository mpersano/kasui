#include <cassert>

#include <guava2d/pixmap.h>
#include <guava2d/texture_manager.h>
#include <guava2d/rgb.h>

#include "render.h"
#include "common.h"
#include "background.h"

enum {
	GRADIENT_TEXTURE_WIDTH = 32,
	GRADIENT_TEXTURE_HEIGHT = 64,
};

static g2d::texture *gradient_texture = nullptr;

void
background_draw_gradient()
{
	render::set_blend_mode(blend_mode::NO_BLEND);

	const float du = 1./GRADIENT_TEXTURE_WIDTH;
	const float u0 = du;
	const float u1 = 1. - du;

	const float dv = 1./GRADIENT_TEXTURE_HEIGHT;
	const float v0 = dv;
	const float v1 = 1. - dv;

	render::set_color({ 1.f, 1.f, 1.f, 1.f });
	render::draw_quad(gradient_texture,
		{ { 0, 0 }, { 0, window_height }, { window_width, window_height }, { window_width, 0 } },
		{ { u0, v0 }, { u0, v1 }, { u1, v1 }, { u1, v0 } },
		-100);
}

void
background_initialize()
{
	gradient_texture =
		new g2d::texture(new g2d::pixmap(GRADIENT_TEXTURE_WIDTH, GRADIENT_TEXTURE_HEIGHT, g2d::pixmap::RGB), 1);
	g2d::texture_manager::get_instance().put("gradient-texture", gradient_texture);
}

void
background_initialize_gradient(const g2d::rgb& from_color, const g2d::rgb& to_color)
{
	assert(gradient_texture);

	const float x0 = -.4*GRADIENT_TEXTURE_WIDTH;
	const float y0 = 1.2*GRADIENT_TEXTURE_HEIGHT;

	uint8_t *bits = gradient_texture->get_pixmap()->get_bits();

	for (int y = 0; y < GRADIENT_TEXTURE_HEIGHT; y++) {
		for (int x = 0; x < GRADIENT_TEXTURE_WIDTH; x++) {
			const float r = sqrtf((x - x0)*(x - x0) + (y - y0)*(y - y0));

			float s = .5*GRADIENT_TEXTURE_WIDTH/r;
			if (s > 1)
				s = 1;

			g2d::rgb color = from_color + (to_color - from_color)*s;
			*bits++ = color.r;
			*bits++ = color.g;
			*bits++ = color.b;
		}
	}

	gradient_texture->upload_pixmap();
}
