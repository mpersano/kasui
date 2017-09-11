#include <cassert>

#include "guava2d/g2dgl.h"
#include "guava2d/font_manager.h"
#include "guava2d/vertex_array.h"
#include "guava2d/texture.h"
#include "guava2d/rgb.h"

#include "common.h"
#include "program_registry.h"
#include "in_game.h"
#include "score_display.h"

enum {
	DIGIT_WIDTH = 36,
	START_DIGIT_BUMP = 10*MS_PER_TIC,
	VALUE_DEQUEUE_TICS = 5*MS_PER_TIC,
};

score_display::score_display()
{
	reset();

	const g2d::font *font = g2d::font_manager::get_instance().load("fonts/medium");
	for (int i = 0; i < 10; i++)
		digit_glyphs_[i] = font->find_glyph(L'0' + i);

	texture_ = font->get_texture();
}

void
score_display::reset()
{
	for (digit *p = digits; p != &digits[NUM_DIGITS]; p++) {
		p->value = 0;
		p->bump = 0;
	}

	queue_head = queue_tail = queue_size = 0;
}

void
score_display::set_value(int value)
{
	for (digit *p = digits; p != &digits[NUM_DIGITS] && value; p++, value /= 10) {
		int d = value%10;

		if (d != p->value) {
			p->value = d;
			p->bump = START_DIGIT_BUMP;
		}
	}
}

void
score_display::enqueue_value(int value)
{
	assert(queue_size < MAX_QUEUE_SIZE);

	queue[queue_tail].tics = VALUE_DEQUEUE_TICS;
	queue[queue_tail].value = value;

	if (++queue_tail == MAX_QUEUE_SIZE)
		queue_tail = 0;
	++queue_size;
}

void
score_display::update(uint32_t dt)
{
	for (digit *p = digits; p != &digits[NUM_DIGITS]; p++) {
		if (p->bump)
			p->bump -= dt;
	}

	if (queue_size > 0) {
		if ((queue[queue_head].tics -= dt) <= 0) {
			set_value(queue[queue_head].value);
			if (++queue_head == MAX_QUEUE_SIZE)
				queue_head = 0;
			--queue_size;
		}
	}
}

void
score_display::draw(const g2d::mat4& proj_modelview, float x_center, float y_center) const
{
	using namespace g2d::vertex;

	typedef g2d::indexed_vertex_array<
			GLubyte,
			attrib<GLfloat, 2>,
			attrib<GLfloat, 2> > vertex_array_type;

	const float SCORE_SCALE = .8;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int num_digits = NUM_DIGITS;

	while (num_digits > 1 && digits[num_digits - 1].value == 0)
		--num_digits;

	static vertex_array_type gv(NUM_DIGITS*4, NUM_DIGITS*6);
	gv.reset();

	int vert_index = 0;

	for (const digit *p = digits; p != &digits[num_digits]; p++) {
		const float fbump = p->bump > 0 ? static_cast<float>(p->bump)/MS_PER_TIC : 0;

		const float scale = SCORE_SCALE/(1. - .05*fbump);

		const g2d::glyph_info *gi = digit_glyphs_[p->value];

		const float dx = .5*gi->width*scale;
		const float dy = .5*gi->height*scale;

		const float x_left = x_center - dx, x_right = x_center + dx;
		const float y_top = y_center + dy, y_bottom = y_center - dy;

		gv << x_left, y_top, gi->texuv[0].x, gi->texuv[0].y;
		gv << x_right, y_top, gi->texuv[1].x, gi->texuv[1].y;
		gv << x_right, y_bottom, gi->texuv[2].x, gi->texuv[2].y;
		gv << x_left, y_bottom, gi->texuv[3].x, gi->texuv[3].y;

		gv < vert_index + 0, vert_index + 1, vert_index + 2;
		gv < vert_index + 2, vert_index + 3, vert_index + 0;

		vert_index += 4;
		x_center -= DIGIT_WIDTH*SCORE_SCALE;
	}

	program_timer_text& prog = get_program_instance<program_timer_text>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);
	prog.set_outline_color(g2d::rgba(1, 1, 1, 1));
	prog.set_text_color(g2d::rgba(0, 0, 0, 1));
	prog.set_texture(0);

	texture_->bind();
	gv.draw(GL_TRIANGLES);
}
