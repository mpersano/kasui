#include <cassert>

#include <guava2d/font_manager.h>
#include <guava2d/rgb.h>

#include "render.h"
#include "common.h"
#include "in_game.h"
#include "timer_display.h"


timer_display::timer_display()
{
	const g2d::font *font = g2d::font_manager::get_instance().load("fonts/medium");

	for (int i = 0; i < 10; i++)
		digit_glyphs_[i] = font->find_glyph(L'0' + i);

	byou_ = font->find_glyph(L'秒');

	texture_ = font->get_texture();
}

void timer_display::draw_glyph(const g2d::glyph_info *g, float x, float y, float scale) const
{
	const float y0 = y + .5*g->height*scale;
	const float y1 = y - .5*g->height*scale;

	const float x0 = x - .5*scale*g->width;
	const float x1 = x + .5*scale*g->width;

	render::draw_quad(
		texture_,
		{ { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } },
		{ g->texuv[0], g->texuv[1], g->texuv[2], g->texuv[3] },
		50);
}

void
timer_display::reset(int tics)
{
	tics_left = tics;
}

void
timer_display::update(uint32_t dt)
{
	if (tics_left > 0) {
		if ((tics_left -= dt) < 0)
			tics_left = 0;
	}
}

void
timer_display::draw(float alpha) const
{
	int secs = tics_left/1000;
	int csecs = (tics_left%1000)/10;

	constexpr int ALARM_FADE_IN_TICS = 60*MS_PER_TIC;
	constexpr int NUM_INTEGER_DIGITS = 2;
	constexpr int NUM_FRACTIONAL_DIGITS = 2;

	constexpr float FRACTIONAL_DIGITS_SCALE = .7;
	constexpr float DIGIT_WIDTH = 36;
	constexpr float BYOU_WIDTH = 1.8*DIGIT_WIDTH;

	constexpr float width =
		NUM_INTEGER_DIGITS*DIGIT_WIDTH +
		FRACTIONAL_DIGITS_SCALE*(NUM_FRACTIONAL_DIGITS*DIGIT_WIDTH + BYOU_WIDTH);

	render::set_blend_mode(blend_mode::ALPHA_BLEND);
	render::set_color({ 1.f, 1.f, 1.f, alpha });

	float mix;

	if (tics_left < MIN_SAFE_SECS*1000 + ALARM_FADE_IN_TICS) {
		float s = .5;

		if (tics_left >= MIN_SAFE_SECS*1000)
			s *= 1. - static_cast<float>(tics_left - MIN_SAFE_SECS*1000)/ALARM_FADE_IN_TICS;

		mix = (1. - s) + s*sin(.2*total_tics/MS_PER_TIC);
	} else {
		mix = 1;
	}

	float x = .5*window_width - .5*width + .5*DIGIT_WIDTH;

	draw_glyph(digit_glyphs_[secs/10], x, 0, 1.);
	x += DIGIT_WIDTH;

	draw_glyph(digit_glyphs_[secs%10], x, 0, 1.);
	x += .5*(DIGIT_WIDTH + FRACTIONAL_DIGITS_SCALE*BYOU_WIDTH);

	draw_glyph(byou_, x, 0, FRACTIONAL_DIGITS_SCALE);
	x += .5*FRACTIONAL_DIGITS_SCALE*(BYOU_WIDTH + DIGIT_WIDTH);

	draw_glyph(digit_glyphs_[csecs/10], x, 0, FRACTIONAL_DIGITS_SCALE);
	x += FRACTIONAL_DIGITS_SCALE*DIGIT_WIDTH;

	draw_glyph(digit_glyphs_[csecs%10], x, 0, FRACTIONAL_DIGITS_SCALE);

#if 0
	program_timer_text& prog = get_program_instance<program_timer_text>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);
	prog.set_texture(0);
	prog.set_outline_color(g2d::rgba(1, mix, mix, alpha));
	prog.set_text_color(g2d::rgba(0, 0, 0, alpha));

	texture_->bind();
	va.draw(GL_TRIANGLES);
#endif
}
