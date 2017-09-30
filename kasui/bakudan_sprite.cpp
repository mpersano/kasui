#include <cassert>
#include <cmath>

#include "guava2d/g2dgl.h"
#include "guava2d/rgb.h"
#include "guava2d/vertex_array.h"
#include "guava2d/texture_manager.h"

#include "common.h"
#include "program_registry.h"
#include "bakudan_sprite.h"

enum {
	TTL = 70*MS_PER_TIC,
	GLOW_TEXTURE_SIZE = 64,
};

bakudan_sprite::bakudan_sprite(float x, float y)
: x_center(x)
, y_center(y)
, start_angle(frand(0., M_PI))
, tics(0)
, glow_texture(g2d::texture_manager::get_instance().load("images/glow.png"))
{ }

bakudan_sprite::~bakudan_sprite()
{ }

bool
bakudan_sprite::update(uint32_t dt)
{
	return (tics += dt) < TTL;
}

void
bakudan_sprite::draw() const
{
#ifdef FIX_ME
	enum {
		NUM_RAYS = 9
	};

	static const float MIN_RADIUS = 100;
	static const float MAX_RADIUS = 600;

	const float lerp_factor = sinf(static_cast<float>(tics)*M_PI/TTL);

	const float radius_rays = MIN_RADIUS + lerp_factor*(MAX_RADIUS - MIN_RADIUS);
	const float radius_glow = .6*radius_rays;

	float angle = start_angle + .01*tics/MS_PER_TIC + .6*lerp_factor;

	const float delta_angle = 2.*M_PI/NUM_RAYS;
	const float fray = .6 - .4*lerp_factor;

	const int ray_alpha = 255*powf(lerp_factor, 5);

	static g2d::vertex_array_color gv_rays(NUM_RAYS*3);
	gv_rays.reset();

	for (int i = 0; i < NUM_RAYS; i++) {
		const float x0 = x_center + cosf(angle)*radius_rays;
		const float y0 = y_center + sinf(angle)*radius_rays;

		const float x1 = x_center + cosf(angle + fray*delta_angle)*radius_rays;
		const float y1 = y_center + sinf(angle + fray*delta_angle)*radius_rays;

		gv_rays << x_center, y_center, ray_alpha, ray_alpha, ray_alpha, ray_alpha;
		gv_rays << x0, y0, 0, 0, 0, 0;
		gv_rays << x1, y1, 0, 0, 0, 0;

		angle += delta_angle;
	}

	assert(gv_rays.get_num_verts() == NUM_RAYS*3);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	{
	program_color& prog = get_program_instance<program_color>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);

	gv_rays.draw(GL_TRIANGLES);
	}

	static g2d::vertex_array_texuv gv_glow(4);
	gv_glow.reset();

	const float x0 = x_center - radius_glow;
	const float x1 = x_center + radius_glow;

	const float y0 = y_center - radius_glow;
	const float y1 = y_center + radius_glow;

	const float glow_alpha = lerp_factor;

	const float du = glow_texture->get_u_scale();
	const float dv = glow_texture->get_v_scale();

	gv_glow << x0, y0, 0, 0;
	gv_glow << x1, y0, du, 0;
	gv_glow << x0, y1, 0, dv;
	gv_glow << x1, y1, du, dv;

	glow_texture->bind();

	{
	program_texture_uniform_alpha& prog = get_program_instance<program_texture_uniform_alpha>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);
	prog.set_texture(0);
	prog.set_alpha(glow_alpha);

	gv_glow.draw(GL_TRIANGLE_STRIP);
	}
#endif
}
