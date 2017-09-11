#include "guava2d/texture_manager.h"
#include "guava2d/sprite_manager.h"

#include "common.h"
#include "program_registry.h"
#include "tween.h"
#include "title_background.h"

title_widget::title_widget()
: cur_state(INSIDE)
, state_t(0)
{ }

void
title_widget::reset()
{
	set_state(ENTERING);
}

void
title_widget::show()
{
	if (cur_state != INSIDE && cur_state != ENTERING)
		set_state(ENTERING);
}

void
title_widget::hide()
{
	if (cur_state != OUTSIDE && cur_state != LEAVING)
		set_state(LEAVING);
}

void
title_widget::set_state(state next_state)
{
	cur_state = next_state;
	state_t = 0;
}

void
title_widget::update(uint32_t dt)
{
	state_t += dt;

	switch (cur_state) {
		case ENTERING:
			if (state_t >= ENTER_T)
				set_state(INSIDE);
			break;

		case LEAVING:
			if (state_t >= LEAVE_T)
				set_state(OUTSIDE);
			break;

		default:
			break;
	}
}

struct foreground_billboard : title_widget
{
	foreground_billboard();

	void draw(const g2d::mat4& proj_modelview) const;

	const g2d::sprite *bb;
	float y;
	float scale;
};

foreground_billboard::foreground_billboard()
: bb(g2d::sprite_manager::get_instance().get_sprite("haru-wo-bg.png"))
{
	const int w = bb->get_width();
	const int h = bb->get_height();

	float scaled_height = window_width*h/w;

	y = .5*(window_height - scaled_height);
	if (y > 0)
		y = 0;

	scale = static_cast<float>(h)/scaled_height;
}

void
foreground_billboard::draw(const g2d::mat4& proj_modelview) const
{
	if (cur_state == OUTSIDE)
		return;

	static const float x_from = -window_width, x_to = 0;
	float x, alpha;

	switch (cur_state) {
		case ENTERING:
			{
			const float t = static_cast<float>(state_t)/ENTER_T;
			x = in_cos_tween<float>()(x_from, x_to, t);
			alpha = t;
			}
			break;

		case LEAVING:
			{
			const float t = static_cast<float>(state_t)/LEAVE_T;
			x = in_back_tween<float>()(x_to, x_from, t);
			alpha = 1. - t;
			}
			break;

		case INSIDE:
			x = x_to;
			alpha = 1;
			break;

		default:
			assert(0);
	}

	g2d::mat4 mat = proj_modelview*g2d::mat4::translation(x, y, 0)*g2d::mat4::scale(scale);

	program_texture_uniform_alpha& prog = get_program_instance<program_texture_uniform_alpha>();
	prog.use();
	prog.set_proj_modelview_matrix(mat);
	prog.set_texture(0);
	prog.set_alpha(alpha);

	bb->draw(0, 0);
}

title_background::title_background()
: fg_billboard(new foreground_billboard)
, logo(new kasui_logo)
, bg_texture(g2d::texture_manager::get_instance().load("images/haru-bg.png"))
, bg_billboard(4)
{
	initialize_bg_billboard();
}

title_background::~title_background()
{
	delete fg_billboard;
	delete logo;
}

void
title_background::initialize_bg_billboard()
{
	const int w = bg_texture->get_pixmap_width();
	const int h = bg_texture->get_pixmap_height();

	float scaled_height = window_width*h/w;
	if (scaled_height < window_height)
		scaled_height = window_height;

	const float dv = bg_texture->get_v_scale();
	const float du = bg_texture->get_u_scale();

	const float y1 = .5*(window_height - scaled_height);
	const float y0 = y1 + scaled_height;

	bg_billboard << 0, y0, 0, 0;
	bg_billboard << 0, y1, 0, dv;
	bg_billboard << window_width, y0, du, 0;
	bg_billboard << window_width, y1, du, dv;
}

void
title_background::reset()
{
	sakura.reset();

	fg_billboard->reset();
	logo->reset();
}

void
title_background::update(uint32_t dt)
{
	sakura.update(dt);
	fg_billboard->update(dt);
	logo->update(dt);
}

void
title_background::draw(const g2d::mat4& proj_modelview) const
{
	glDisable(GL_BLEND);

	// bg_billboard

	program_texture_decal& prog = get_program_instance<program_texture_decal>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);
	prog.set_texture(0);

	bg_texture->bind();
	bg_billboard.draw(GL_TRIANGLE_STRIP);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	fg_billboard->draw(proj_modelview);

	logo->draw(proj_modelview);

	sakura.draw(proj_modelview);
}
