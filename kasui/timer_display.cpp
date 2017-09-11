#include <cassert>

#include "guava2d/g2dgl.h"
#include "guava2d/texture.h"
#include "guava2d/font_manager.h"
#include "guava2d/vertex_array.h"
#include "guava2d/rgb.h"

#include "program_registry.h"
#include "common.h"
#include "in_game.h"
#include "tween.h"
#include "action.h"
#include "sounds.h"
#include "timer_display.h"


timer_display::timer_display()
{
	const g2d::font *font = g2d::font_manager::get_instance().load("fonts/medium");

	for (int i = 0; i < 10; i++)
		digit_glyphs_[i] = font->find_glyph(L'0' + i);

	byou_ = font->find_glyph(L'秒');

	texture_ = font->get_texture();
}

typedef g2d::indexed_vertex_array<
		GLubyte,
		g2d::vertex::attrib<GLfloat, 2>,
		g2d::vertex::attrib<GLfloat, 2> > vertex_array_type;

static void
draw_glyph(vertex_array_type& va, const g2d::glyph_info *g, float x, float y, float scale)
{
	const float y0 = y + .5*g->height*scale;
	const float y1 = y - .5*g->height*scale;

	const float x0 = x - .5*scale*g->width;
	const float x1 = x + .5*scale*g->width;

	const float t0x = g->texuv[0].x;
	const float t0y = g->texuv[0].y;
	
	const float t1x = g->texuv[1].x;
	const float t1y = g->texuv[1].y;
	
	const float t2x = g->texuv[2].x;
	const float t2y = g->texuv[2].y;
	
	const float t3x = g->texuv[3].x;
	const float t3y = g->texuv[3].y;

	const int vert_index = va.get_num_verts();

	va << x0, y0, t0x, t0y;
	va << x1, y0, t1x, t1y;
	va << x1, y1, t2x, t2y;
	va << x0, y1, t3x, t3y;

	va < vert_index + 0, vert_index + 1, vert_index + 2,
	     vert_index + 2, vert_index + 3, vert_index + 0;
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
timer_display::draw(const g2d::mat4& proj_modelview, float alpha) const
{
	int secs = tics_left/1000;
	int csecs = (tics_left%1000)/10;

	enum {
		ALARM_FADE_IN_TICS = 60*MS_PER_TIC,
		NUM_INTEGER_DIGITS = 2,
		NUM_FRACTIONAL_DIGITS = 2
	};

	static const float FRACTIONAL_DIGITS_SCALE = .7;
	static const float DIGIT_WIDTH = 36;
	static const float BYOU_WIDTH = 1.8*DIGIT_WIDTH;

	const float width =
		NUM_INTEGER_DIGITS*DIGIT_WIDTH +
		FRACTIONAL_DIGITS_SCALE*(NUM_FRACTIONAL_DIGITS*DIGIT_WIDTH + BYOU_WIDTH);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

	static vertex_array_type va(5*4, 5*6);
	va.reset();

	draw_glyph(va, digit_glyphs_[secs/10], x, 0, 1.);
	x += DIGIT_WIDTH;

	draw_glyph(va, digit_glyphs_[secs%10], x, 0, 1.);
	x += .5*(DIGIT_WIDTH + FRACTIONAL_DIGITS_SCALE*BYOU_WIDTH);

	draw_glyph(va, byou_, x, 0, FRACTIONAL_DIGITS_SCALE);
	x += .5*FRACTIONAL_DIGITS_SCALE*(BYOU_WIDTH + DIGIT_WIDTH);

	draw_glyph(va, digit_glyphs_[csecs/10], x, 0, FRACTIONAL_DIGITS_SCALE);
	x += FRACTIONAL_DIGITS_SCALE*DIGIT_WIDTH;

	draw_glyph(va, digit_glyphs_[csecs%10], x, 0, FRACTIONAL_DIGITS_SCALE);

	program_timer_text& prog = get_program_instance<program_timer_text>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);
	prog.set_texture(0);
	prog.set_outline_color(g2d::rgba(1, mix, mix, alpha));
	prog.set_text_color(g2d::rgba(0, 0, 0, alpha));

	texture_->bind();
	va.draw(GL_TRIANGLES);
}
