#include <guava2d/font_manager.h>

#include "program_manager.h"
#include "render.h"
#include "jukugo.h"
#include "common.h"
#include "tween.h"
#include "line_splitter.h"
#include "jukugo_info_sprite.h"

jukugo_info_sprite::jukugo_info_sprite(const jukugo *jukugo_info, float x, float y, const gradient *g)
	: jukugo_(jukugo_info)
	, x_base_(x)
	, y_base_(y)
	, gradient_(g)
{
	action_.reset(
	  (new parallel_action_group)
	    ->add(
	      (new sequential_action_group)
	        ->add(new property_change_action<out_bounce_tween<float>>(&flip_, 0, .5*M_PI, 30*MS_PER_TIC))->add(new delay_action(30*MS_PER_TIC)))
	    ->add(
	      (new sequential_action_group)
	        ->add(new delay_action(60*MS_PER_TIC))->add(new property_change_action<in_back_tween<float>>(&z_, 0, 1, 20*MS_PER_TIC)))
	    ->add(
	      (new sequential_action_group)
	        ->add(new property_change_action<linear_tween<float>>(&alpha_, 0, 1., 10*MS_PER_TIC))
		->add(new delay_action(60*MS_PER_TIC))
		->add(new property_change_action<linear_tween<float>>(&alpha_, 1., 0, 10*MS_PER_TIC))));

	kanji_font_ = g2d::font_manager::get_instance().load("fonts/large");
	eigo_font_ = g2d::font_manager::get_instance().load("fonts/tiny");
	furigana_font_ = g2d::font_manager::get_instance().load("fonts/small");

	pos_kanji_ = { -.5f*kanji_font_->get_string_width(jukugo_info->kanji), -20.f };
	pos_furigana_ = { -.5f*furigana_font_->get_string_width(jukugo_info->reading), 64.f };

	constexpr int MAX_LINE_WIDTH = 320;
	float y_eigo = -64;

	line_splitter ls(eigo_font_, jukugo_info->eigo);

	std::wstring line;
	while (line = ls.next_line(MAX_LINE_WIDTH), !line.empty()) {
		const float x = -.5*eigo_font_->get_string_width(line.c_str());
		eigo_lines_.emplace_back(g2d::vec2(x, y_eigo), std::move(line));
		y_eigo -= 44;
	}
}

bool
jukugo_info_sprite::update(uint32_t dt)
{
	action_->step(dt);
	return !action_->done();
}

void
jukugo_info_sprite::draw() const
{
	const float scale = 1./(1. + z_);
	const float sy = sinf(flip_);

	render::set_blend_mode(blend_mode::ALPHA_BLEND);

	render::push_matrix();

	render::translate(x_base_, y_base_);
	render::scale(scale, scale*sy);

	render::set_color({ 1.f, 1.f, 1.f, alpha_ });

	render::draw_text(kanji_font_, pos_kanji_, 50, jukugo_->kanji);
	render::draw_text(furigana_font_, pos_furigana_, 50, jukugo_->reading);

	for (const auto& p : eigo_lines_)
		render::draw_text(eigo_font_, p.first, 50, p.second.c_str());

	render::pop_matrix();

#if 0
	const float scale = 1./(1. + z);
	const float sy = sinf(flip);

	const g2d::mat4 mat =
		proj_modelview*
		g2d::mat4::translation(x_base, y_base, 0)*
		g2d::mat4::scale(scale, scale*sy, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const g2d::rgb& top_color = *gradient_->from;
	const g2d::rgb& bottom_color = *gradient_->to;

	const g2d::rgb top_color_text = top_color*(1./255);
	const g2d::rgb bottom_color_text = bottom_color*(1./255);

	const g2d::rgb top_color_outline = .5*top_color_text;
	const g2d::rgb bottom_color_outline = .5*bottom_color_text;

	// outline

	{
	program_text_outline_gradient& prog = get_program_instance<program_text_outline_gradient>();

	prog.use();
	prog.set_proj_modelview_matrix(mat);
	prog.set_texture(0);
	prog.set_top_color(g2d::rgba(top_color_outline, alpha));
	prog.set_bottom_color(g2d::rgba(bottom_color_outline, alpha));

	draw_quads();
	}

	// text

	{
	program_text_gradient& prog = get_program_instance<program_text_gradient>();

	prog.use();
	prog.set_proj_modelview_matrix(mat);
	prog.set_texture(0);
	prog.set_top_color(g2d::rgba(top_color_text, alpha));
	prog.set_bottom_color(g2d::rgba(bottom_color_text, alpha));

	draw_quads();
	}
#endif
}
