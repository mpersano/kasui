#ifndef DRAW_QUEUE_H_
#define DRAW_QUEUE_H_

#include <vector>
#include <functional>

#include "g2dgl.h"
#include "vec2.h"

namespace g2d {

class draw_queue_impl;

class program;
class font;

class draw_queue
{
public:
	draw_queue();
	~draw_queue();

	// transformations

	draw_queue& set_matrix(const mat3& mat);
	draw_queue& translate(const vec2& pos);
	draw_queue& translate(float x, float y);
	draw_queue& scale(float s);
	draw_queue& rotate(float a);
	draw_queue& push_matrix();
	draw_queue& pop_matrix();

	// text rendering

	draw_queue& align_left();
	draw_queue& align_right();
	draw_queue& align_center();

	draw_queue& enable_text_outline();
	draw_queue& disable_text_outline();

	draw_queue& text_program(const program *prog);
	draw_queue& text_outline_program(const program *prog);

	draw_queue& render_text(const font *f, const wchar_t *fmt, ...);

	void draw() const;

	void reset();

private:
	draw_queue_impl *impl_;
};

}

#endif // DRAW_QUEUE_H_
