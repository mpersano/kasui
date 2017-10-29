#include <sstream>

#include <guava2d/font_manager.h>

#include "render.h"
#include "common.h"
#include "theme.h"
#include "combo_sprite.h"


combo_sprite::combo_sprite(int combo_size, float x, float y, const gradient *g)
	: x_origin_(x)
	, y_origin_(y)
	, gradient_(g)
	, large_font_(g2d::font_manager::get_instance().load("fonts/large"))
	, small_font_(g2d::font_manager::get_instance().load("fonts/small"))
{
	// XXX use boost::lexical_cast here
	std::wstringstream ss;
	ss << combo_size;
	combo_size_ = ss.str();

	x_chain_text_ = large_font_->get_string_width(combo_size_.c_str()) + 12;
}

bool
combo_sprite::update(uint32_t dt)
{
	float dy = cosf(ttl_*M_PI/TTL);
	dy *= dy;
	dy += .4;

	y_offset_ += dt*2.4*dy/MS_PER_TIC;

	return (ttl_ -= dt) > 0;
}

void
combo_sprite::draw() const
{
	float alpha = sinf(ttl_*M_PI/TTL);
	alpha *= alpha;

	render::set_blend_mode(blend_mode::ALPHA_BLEND);
	render::set_color({ 1.f, 1.f, 1.f, alpha });

	render::push_matrix();
	render::translate(x_origin_, y_origin_ + y_offset_);

	render::draw_text(large_font_, {}, combo_size_.c_str());
	render::draw_text(small_font_, { x_chain_text_, 16 }, L"chain!");

	render::pop_matrix();

#if 0
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float alpha = sin(ttl*M_PI/TTL);
	alpha *= alpha;

	const g2d::mat4 mat =
		proj_modelview*
		g2d::mat4::translation(x_origin, y_origin + y_offset, 0);

	const g2d::rgb& text_color = *gradient_->to*(1./255.);
	const g2d::rgb& outline_color = .5*text_color;

	program_timer_text& prog = get_program_instance<program_timer_text>();
	prog.use();
	prog.set_proj_modelview_matrix(mat);
	prog.set_texture(0);
	prog.set_text_color(g2d::rgba(text_color, alpha));
	prog.set_outline_color(g2d::rgba(outline_color, alpha));

	text_.draw();
#endif
}
