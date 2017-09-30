#include <algorithm>

#include <cstring>

#include "guava2d/g2dgl.h"
#include "guava2d/font_manager.h"
#include "guava2d/texture.h"
#include "guava2d/program.h"
#include "guava2d/vertex_array.h"
#include "guava2d/xwchar.h"

#include "settings.h"
#include "program_registry.h"
#include "jukugo.h"
#include "common.h"
#include "tween.h"
#include "line_splitter.h"
#include "jukugo_info_sprite.h"

jukugo_info_sprite::jukugo_info_sprite(const jukugo *jukugo_info, float x, float y, const gradient *g)
: x_base(x)
, y_base(y)
, flip(0)
, alpha(0)
, z(0)
, gradient_(g)
{
	action =
	  (new parallel_action_group)
	    ->add(
	      (new sequential_action_group)
	        ->add(new property_change_action<out_bounce_tween<float> >(&flip, 0, .5*M_PI, 30*MS_PER_TIC))->add(new delay_action(30*MS_PER_TIC)))
	    ->add(
	      (new sequential_action_group)
	        ->add(new delay_action(60*MS_PER_TIC))->add(new property_change_action<in_back_tween<float> >(&z, 0, 1, 20*MS_PER_TIC)))
	    ->add(
	      (new sequential_action_group)
	        ->add(new property_change_action<linear_tween<float> >(&alpha, 0, 1., 10*MS_PER_TIC))
		->add(new delay_action(60*MS_PER_TIC))
		->add(new property_change_action<linear_tween<float> >(&alpha, 1., 0, 10*MS_PER_TIC)));

	initialize_quads(jukugo_info);
}

jukugo_info_sprite::~jukugo_info_sprite()
{
	delete action;
}

void
jukugo_info_sprite::initialize_quads(const jukugo *jukugo_info)
{
	const g2d::font *large_font = g2d::font_manager::get_instance().load("fonts/large");
	const g2d::font *tiny_font = g2d::font_manager::get_instance().load("fonts/tiny");
	const g2d::font *small_font = g2d::font_manager::get_instance().load("fonts/small");

	const float x_kanji = -.5*large_font->get_string_width(jukugo_info->kanji);
	const float y_kanji = -20;

	const float x_furigana = -.5*small_font->get_string_width(jukugo_info->reading);
	const float y_furigana = 64;

	const float y_eigo = -64;

	// kanji (large font)

	initialize_string_quads(
		large_font,
		x_kanji, y_kanji,
		1, 0,
		jukugo_info->kanji);

	// furigana (small font)

	initialize_string_quads(
		small_font,
		x_furigana, y_furigana,
		1, 1,
		jukugo_info->reading);

	// english (tiny font)

	static const int MAX_LINE_WIDTH = 320;

	line_splitter ls(tiny_font, jukugo_info->eigo);

	float y = y_eigo;
	wchar_t *line;

	while ((line = ls.next_line(MAX_LINE_WIDTH))) {
		const float x = -.5*tiny_font->get_string_width(line);
		initialize_string_quads(tiny_font, x, y, 0, 0, line);
		delete[] line;
		y -= 44;
	}
}

void
jukugo_info_sprite::initialize_string_quads(
	const g2d::font *font,
	float x, float y,
	float shade_top, float shade_bottom,
	const wchar_t *str)
{
	using namespace g2d;

	for (const wchar_t *p = str; *p; p++) {
		const glyph_info *g = font->find_glyph(*p);

		const vec2& t0 = g->texuv[0];
		const vec2& t1 = g->texuv[1];
		const vec2& t2 = g->texuv[2];
		const vec2& t3 = g->texuv[3];

		const float x_left = x + g->left;
		const float x_right = x_left + g->width;
		const float y_top = y + g->top;
		const float y_bottom = y_top - g->height;

		const vec2 p0 = vec2(x_left, y_top);
		const vec2 p1 = vec2(x_right, y_top);
		const vec2 p2 = vec2(x_right, y_bottom);
		const vec2 p3 = vec2(x_left, y_bottom);

		vertex_array& gv = quads_[g->texture_];

		const int vert_index = gv.get_num_verts();

		gv << p0.x, p0.y, t0.x, t0.y, shade_top;
		gv << p1.x, p1.y, t1.x, t1.y, shade_top;
		gv << p2.x, p2.y, t2.x, t2.y, shade_bottom;
		gv << p3.x, p3.y, t3.x, t3.y, shade_bottom;

		gv < vert_index + 0, vert_index + 1, vert_index + 2,
		     vert_index + 2, vert_index + 3, vert_index + 0;

		x += g->advance_x;
	}
}

bool
jukugo_info_sprite::update(uint32_t dt)
{
	action->step(dt);
	return !action->done();
}

void
jukugo_info_sprite::draw_quads() const
{
	std::for_each(
		quads_.begin(),
		quads_.end(),
		[this] (const map_value_type& v) {
			v.first->bind();
			v.second.draw(GL_TRIANGLES);
		});
}

void
jukugo_info_sprite::draw() const
{
#ifdef FIX_ME
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
