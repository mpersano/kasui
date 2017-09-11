#include <map>
#include <vector>
#include <algorithm>

#include "guava2d/vec2.h"
#include "guava2d/vec3.h"
#include "guava2d/vertex_array.h"
#include "guava2d/font.h"
#include "guava2d/texture.h"
#include "guava2d/font_manager.h"
#include "guava2d/rgb.h"

#include "program_registry.h"
#include "common.h"
#include "main_menu.h"
#include "credits.h"

class credits_impl
{
public:
	credits_impl();

	void redraw() const;
	void update(uint32_t dt);
	void reset();

	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_touch_move(float x, float y);
	void on_back_key();
	void on_menu_key();

private:
	void add_text(const g2d::font *font, float x_base, float y_base, int start_tic, const char *text);

	struct particle {
		void draw(g2d::vertex_array_texuv_alpha& gv, float t, float alpha) const;

		float x_left, x_right;
		float y_top, y_bottom;
		g2d::vec2 uv0, uv1, uv2, uv3;
		int start_tic;
	};

	void add_glyph_fragments(
		std::vector<particle>& particles,
		float x_left, float x_right,
		float y_top, float y_bottom,
		const g2d::vec2& uv0, const g2d::vec2& uv1,
		const g2d::vec2& uv2, const g2d::vec2& uv3,
		int start_tic, int depth);

	void draw_particles(float alpha) const;
	void draw_particles(const std::vector<particle>& particles, float t, float alpha) const;

	typedef std::map<const g2d::texture *, std::vector<particle> > map_type;
	typedef map_type::value_type map_value_type;

	map_type particles_;

	int tics_;

	enum {
		INTRO,
		IDLE,
		OUTRO,
	} cur_state_;

	enum {
		INTRO_TICS = 30*MS_PER_TIC,
		OUTRO_TICS = 20*MS_PER_TIC,
	};

	int state_tics_;
};


credits_impl::credits_impl()
{
	const g2d::font *tiny_font = g2d::font_manager::get_instance().load("fonts/tiny");
	const g2d::font *small_font = g2d::font_manager::get_instance().load("fonts/small");

	add_text(small_font, 60, 560, 5*MS_PER_TIC, "Mauro Persano");
	add_text(tiny_font, 100, 520, 15*MS_PER_TIC, "programming, game design,");
	add_text(tiny_font, 100, 480, 25*MS_PER_TIC, "some graphics");

	add_text(small_font, 60, 360, 40*MS_PER_TIC, "Mitsue Sumitani");
	add_text(tiny_font, 100, 320, 50*MS_PER_TIC, "artwork, game design,");
	add_text(tiny_font, 100, 280, 60*MS_PER_TIC, "ideas");

	reset();
}

void
credits_impl::add_text(const g2d::font *font, float x_base_orig, float y_base_orig, int start_tic, const char *text)
{
	float x_base = x_base_orig;
	float y_base = y_base_orig;

	for (const char *p = text; *p; p++) {
		const g2d::glyph_info *g = font->find_glyph(*p);

		const float x_left = x_base + g->left;
		const float x_right = x_base + g->left + g->width;

		const float y_top = y_base + g->top;
		const float y_bottom = y_base + g->top - g->height;

		add_glyph_fragments(
			particles_[g->texture_],
			x_left, x_right,
			y_top, y_bottom,
			g->texuv[0], g->texuv[1],
			g->texuv[2], g->texuv[3],
			start_tic, 2);

		x_base += g->advance_x;
	}
}

void
credits_impl::add_glyph_fragments(
	std::vector<particle>& particles,
	float x_left, float x_right,
	float y_top, float y_bottom,
	const g2d::vec2& uv0, const g2d::vec2& uv1,
	const g2d::vec2& uv2, const g2d::vec2& uv3,
	int start_tic, int depth)
{
	if (depth == 0) {
		particle p;

		p.x_left = x_left;
		p.x_right = x_right;
		p.y_top = y_top;
		p.y_bottom = y_bottom;

		p.uv0 = uv0;
		p.uv1 = uv1;
		p.uv2 = uv2;
		p.uv3 = uv3;

		p.start_tic = start_tic;

		particles.push_back(p);
	} else {
		g2d::vec2 uv01 = .5*(uv0 + uv1);
		g2d::vec2 uv12 = .5*(uv1 + uv2);
		g2d::vec2 uv03 = .5*(uv0 + uv3);
		g2d::vec2 uv23 = .5*(uv3 + uv2);

		g2d::vec2 uvm = .5*(uv01 + uv23);

		add_glyph_fragments(particles, x_left, .5*(x_left + x_right), y_top, .5*(y_top + y_bottom), uv0, uv01, uvm, uv03, start_tic, depth - 1);
		add_glyph_fragments(particles, .5*(x_left + x_right), x_right, y_top, .5*(y_top + y_bottom), uv01, uv1, uv12, uvm, start_tic, depth - 1);

		add_glyph_fragments(particles, x_left, .5*(x_left + x_right), .5*(y_top + y_bottom), y_bottom, uv03, uvm, uv23, uv3, start_tic, depth - 1);
		add_glyph_fragments(particles, .5*(x_left + x_right), x_right, .5*(y_top + y_bottom), y_bottom, uvm, uv12, uv2, uv23, start_tic, depth - 1);
	}
}

void
credits_impl::draw_particles(const std::vector<particle>& particles, float t, float alpha) const
{
	static g2d::vertex_array_texuv_alpha gv(512);
	gv.reset();

	std::for_each(
		particles.begin(),
		particles.end(),
		[&] (const particle& p) { p.draw(gv, t, alpha); });

	gv.draw(GL_TRIANGLES);
}

void
credits_impl::particle::draw(g2d::vertex_array_texuv_alpha& gv, float t, float alpha) const
{
	if (t < start_tic)
		return;

	t -= start_tic;

	enum { ANIMATION_TICS = 60*MS_PER_TIC };

	const float x_axis = 200.;
	const float x_origin = 40.;

	const float f = t < ANIMATION_TICS ? 1. - static_cast<float>(t)/ANIMATION_TICS : 0.;
	const float g = .5*1e-2*(x_left - x_origin) + .125*1e-5*y_top*y_top;

	const float x0 = x_axis + (.1*f*f + 1.)*(x_left - x_axis)*cosf(15.*(g + .5)*(g + .5)*f*f);
	const float x1 = x0 + x_right - x_left;

	const float dy = 300.*(2.*f*f*g*g + f*f);

	const float a = (1. - f)*alpha;

	gv << x0, y_top + dy, uv0.x, uv0.y, a;
	gv << x1, y_top + dy, uv1.x, uv1.y, a;
	gv << x1, y_bottom + dy, uv2.x, uv2.y, a;

	gv << x1, y_bottom + dy, uv2.x, uv2.y, a;
	gv << x0, y_bottom + dy, uv3.x, uv3.y, a;
	gv << x0, y_top + dy, uv0.x, uv0.y, a;
}

void
credits_impl::redraw() const
{
	const g2d::mat4 proj_modelview = get_ortho_projection();

	float alpha;

	switch (cur_state_) {
		case INTRO:
			alpha = static_cast<float>(state_tics_)/INTRO_TICS;
			break;

		case OUTRO:
			alpha = 1. - static_cast<float>(state_tics_)/OUTRO_TICS;
			break;

		default:
			alpha = 1;
			break;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	{
	program_text_outline_alpha& prog = get_program_instance<program_text_outline_alpha>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);
	prog.set_texture(0);
	prog.set_color(g2d::rgb(.5, 0., 0.));

	draw_particles(alpha);
	}

	{
	program_text_alpha& prog = get_program_instance<program_text_alpha>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);
	prog.set_texture(0);
	prog.set_color(g2d::rgb(1., 1., 1.));

	draw_particles(alpha);
	}
}

void
credits_impl::draw_particles(float alpha) const
{
	std::for_each(
		particles_.begin(),
		particles_.end(),
		[&] (const map_value_type& p) {
			p.first->bind();
			draw_particles(p.second, tics_, alpha);
		});
}

void
credits_impl::update(uint32_t dt)
{
	tics_ += dt;

	switch (cur_state_) {
		case INTRO:
			if ((state_tics_ += dt) >= INTRO_TICS)
				cur_state_ = IDLE;
			break;

		case OUTRO:
			if ((state_tics_ += dt) >= OUTRO_TICS) {
				// lol
				main_menu_state *main_menu = static_cast<main_menu_state *>(get_prev_state());
				main_menu->show_background();
				main_menu->show_more_menu();

				pop_state();
			}
			break;

		default:
			break;
	}
}

void
credits_impl::reset()
{
	tics_ = state_tics_ = 0;
	cur_state_ = INTRO;
}

void
credits_impl::on_touch_down(float, float)
{
	on_back_key();
}

void
credits_impl::on_touch_up()
{ }

void
credits_impl::on_touch_move(float, float)
{ }

void
credits_impl::on_back_key()
{
	if (cur_state_ == IDLE) {
		cur_state_ = OUTRO;
		state_tics_ = 0;
	}
}

void
credits_impl::on_menu_key()
{ }

credits_state::credits_state()
: impl_(new credits_impl)
{ }

credits_state::~credits_state()
{
	delete impl_;
}

void
credits_state::reset()
{
	impl_->reset();
}

void
credits_state::redraw() const
{
	get_prev_state()->redraw(); // draw main menu background
	impl_->redraw();
}

void
credits_state::update(uint32_t dt)
{
	get_prev_state()->update(dt); // update main menu background
	impl_->update(dt);
}

void
credits_state::on_touch_down(float x, float y)
{
	impl_->on_touch_down(x, y);
}

void
credits_state::on_touch_up()
{
	impl_->on_touch_up();
}

void
credits_state::on_touch_move(float x, float y)
{
	impl_->on_touch_move(x, y);
}

void
credits_state::on_back_key()
{
	impl_->on_back_key();
}

void
credits_state::on_menu_key()
{
	impl_->on_menu_key();
}
