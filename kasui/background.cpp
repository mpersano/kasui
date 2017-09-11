#include <cassert>

#include "guava2d/vertex_array.h"
#include "guava2d/pixmap.h"
#include "guava2d/texture_manager.h"
#include "guava2d/rgb.h"

#include "program_registry.h"
#include "common.h"
#include "background.h"

enum {
	GRADIENT_TEXTURE_WIDTH = 32,
	GRADIENT_TEXTURE_HEIGHT = 64,
};

static g2d::texture *gradient_texture = 0;
static g2d::vertex_array_texuv gradient_verts(4);

void
background_draw_gradient()
{
	glDisable(GL_BLEND);

	gradient_texture->bind();

	program_texture_decal& prog = get_program_instance<program_texture_decal>();
	prog.use();
	prog.set_proj_modelview_matrix(get_ortho_projection());
	prog.set_texture(0);

	gradient_verts.draw(GL_TRIANGLE_STRIP);
}

void
background_initialize()
{
	gradient_texture =
		new g2d::texture(new g2d::pixmap(GRADIENT_TEXTURE_WIDTH, GRADIENT_TEXTURE_HEIGHT, g2d::pixmap::RGB), 1);
	g2d::texture_manager::get_instance().put("gradient-texture", gradient_texture);

	const float du = 1./GRADIENT_TEXTURE_WIDTH;
	const float u0 = du;
	const float u1 = 1. - du;

	const float dv = 1./GRADIENT_TEXTURE_HEIGHT;
	const float v0 = dv;
	const float v1 = 1. - dv;

	gradient_verts << 0, 0, u0, v0;
	gradient_verts << 0, window_height, u0, v1;
	gradient_verts << window_width, 0, u1, v0;
	gradient_verts << window_width, window_height, u1, v1;
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
