#include <algorithm>

#include <guava2d/texture_manager.h>
#include <guava2d/font_manager.h>
#include <guava2d/rgb.h>
#include <guava2d/xwchar.h>

#include "common.h"
#include "line_splitter.h"
#include "program_registry.h"
#include "text_box.h"

namespace {

template <typename Visitor>
void
for_each_char(const wchar_t *text, float max_width, Visitor visit)
{
	const g2d::font *fonts[] =
		{
			g2d::font_manager::get_instance().load("fonts/small"),
			g2d::font_manager::get_instance().load("fonts/tiny"),
		};

	const int font_heights[] =
		{
			46,
			38,
		};

	int cur_font = 0;
	int cur_line_height = font_heights[cur_font];

	float y = 0, x = 0;

	const wchar_t *word_start = 0;
	float word_start_x = 0;

	auto add_word = [&] {
		float x = word_start_x;
		const g2d::font *font = fonts[cur_font];

		for (const wchar_t *q = word_start; *q && (*q != ' ' && *q != '\n' && *q != '@'); q++) {
			const g2d::glyph_info *g = font->find_glyph(*q);
			visit(font, g, x, y);
			x += g->advance_x;
		}

		word_start = 0;

		return x;
	};

	for (const wchar_t *p = text; *p; p++) {
		if (*p == '@') {
			if (word_start)
				add_word();

			cur_font = static_cast<int>(*++p) - '0';

			if (x == 0)
				cur_line_height = font_heights[cur_font];
			else
				cur_line_height = std::max(font_heights[cur_font], cur_line_height);
		} else if (*p == '\n') {
			if (word_start)
				add_word();

			y -= cur_line_height;
			x = 0;
			cur_line_height = font_heights[cur_font];
		} else {
			if (*p == ' ') {
				if (word_start)
					add_word();
			} else {
				if (!word_start) {
					word_start = p;
					word_start_x = x;
				}
			}

			x += fonts[cur_font]->find_glyph(*p)->advance_x;

			if (x > max_width) {
				word_start_x = 0;

				x = 0;

				if (word_start) {
					for (const wchar_t *q = word_start; q <= p; q++)
						x += fonts[cur_font]->find_glyph(*q)->advance_x;
				}

				y -= cur_line_height;
				cur_line_height = font_heights[cur_font];
			}
		}
	}

	if (word_start)
		add_word();
}

};

text_box::text_box(float width)
: width_(width)
, height_(0)
, frame_texture_(g2d::texture_manager::get_instance().load("images/w-button-border.png"))
, frame_va_ { 8, 8, 8 }
{ }

void
text_box::set_text(const wchar_t *text)
{
	quads_.clear();

	const float max_width = width_ - 2*BORDER_RADIUS;

	// figure out bounding box

	bool first = true;

	float x_min = -1, x_max = -1;
	float y_min = -1, y_max = -1;

	for_each_char(text, max_width, [&](const g2d::font *font, const g2d::glyph_info *g, float x, float y) {
		const float x_left = x + g->left;
		const float x_right = x_left + g->width;
		const float y_top = y + g->top;
		const float y_bottom = y_top - g->height;

		if (first) {
			x_min = x_left;
			x_max = x_right;

			y_max = y_top;
			y_min = y_bottom;

			first = false;
		} else {
			x_min = std::min(x_left, x_min);
			x_max = std::max(x_right, x_max);

			y_max = std::max(y_top, y_max);
			y_min = std::min(y_bottom, y_min);
		}
	});

	height_ = y_max - y_min + 2*BORDER_RADIUS;

	const float offs_x = -.5*width_ + BORDER_RADIUS - x_min;
	const float offs_y = -.5*height_ + BORDER_RADIUS - y_min;

	// add glyphs to vertex arrays

	for_each_char(text, max_width, [&](const g2d::font *font, const g2d::glyph_info *g, float x, float y) {
		using g2d::vec2;

		const vec2& t0 = g->texuv[0];
		const vec2& t1 = g->texuv[1];
		const vec2& t2 = g->texuv[2];
		const vec2& t3 = g->texuv[3];

		const float x_left = x + g->left + offs_x;
		const float x_right = x_left + g->width;
		const float y_top = y + g->top + offs_y;
		const float y_bottom = y_top - g->height;

		const vec2 p0 = vec2(x_left, y_top);
		const vec2 p1 = vec2(x_right, y_top);
		const vec2 p2 = vec2(x_right, y_bottom);
		const vec2 p3 = vec2(x_left, y_bottom);

		auto& gv = quads_[g->texture_];

		const int vert_index = gv.get_num_verts();

		gv << p0.x, p0.y, t0.x, t0.y;
		gv << p1.x, p1.y, t1.x, t1.y;
		gv << p2.x, p2.y, t2.x, t2.y;
		gv << p3.x, p3.y, t3.x, t3.y;

		gv < vert_index + 0, vert_index + 1, vert_index + 2,
		     vert_index + 2, vert_index + 3, vert_index + 0;
	});

	initialize_frame();
}

void
text_box::initialize_frame()
{
	// frame vertex array

	const float x0 = -.5*width_;
	const float x1 = x0 + BORDER_RADIUS;

	const float x3 = .5*width_;
	const float x2 = x3 - BORDER_RADIUS;
	
	const float y0 = .5*height_;
	const float y1 = y0 - BORDER_RADIUS;

	const float y3 = -.5*height_;
	const float y2 = y3 + BORDER_RADIUS;

	frame_va_[0].reset();

	frame_va_[0] << x0, y0, 0, 0;
	frame_va_[0] << x0, y1, 0, .25;

	frame_va_[0] << x1, y0, .25, 0;
	frame_va_[0] << x1, y1, .25, .25;

	frame_va_[0] << x2, y0, .75, 0;
	frame_va_[0] << x2, y1, .75, .25;

	frame_va_[0] << x3, y0, 1, 0;
	frame_va_[0] << x3, y1, 1, .25;

	frame_va_[1].reset();

	frame_va_[1] << x0, y1, 0, .25;
	frame_va_[1] << x0, y2, 0, .75;

	frame_va_[1] << x1, y1, .25, .25;
	frame_va_[1] << x1, y2, .25, .75;

	frame_va_[1] << x2, y1, .75, .25;
	frame_va_[1] << x2, y2, .75, .75;

	frame_va_[1] << x3, y1, 1, .25;
	frame_va_[1] << x3, y2, 1, .75;

	frame_va_[2].reset();

	frame_va_[2] << x0, y2, 0, .75;
	frame_va_[2] << x0, y3, 0, 1;

	frame_va_[2] << x1, y2, .25, .75;
	frame_va_[2] << x1, y3, .25, 1;

	frame_va_[2] << x2, y2, .75, .75;
	frame_va_[2] << x2, y3, .75, 1;

	frame_va_[2] << x3, y2, 1, .75;
	frame_va_[2] << x3, y3, 1, 1;
}

void
text_box::draw_frame(const g2d::mat4& mat, float alpha) const
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	frame_texture_->bind();

	for (const auto& va : frame_va_)
		va.draw(GL_TRIANGLE_STRIP);
}

void
text_box::draw(const g2d::mat4& mat, float alpha) const
{
	// draw frame

	{
	auto& prog = get_program_instance<program_texture_uniform_alpha>();
	prog.use();
	prog.set_proj_modelview_matrix(mat);
	prog.set_texture(0);
	prog.set_alpha(alpha);

	draw_frame(mat, alpha);
	}

	// draw text
	{
	auto& prog = get_program_instance<program_texture_uniform_color>();
	prog.use();
	prog.set_proj_modelview_matrix(mat);
	prog.set_texture(0);
	prog.set_color(g2d::rgba(alpha, alpha, alpha, 1));

	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

	std::for_each(
		quads_.begin(),
		quads_.end(),
		[this] (const map_value_type& v) {
			v.first->bind();
			v.second.draw(GL_TRIANGLES);
		});
	}
}
