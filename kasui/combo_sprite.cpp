#include "guava2d/g2dgl.h"
#include "guava2d/vertex_array.h"
#include "guava2d/font_manager.h"
#include "guava2d/xwchar.h"

#include "program_registry.h"
#include "common.h"
#include "theme.h"
#include "combo_sprite.h"

enum {
	TTL = 60*MS_PER_TIC,
	DIGIT_WIDTH = 25,
};

combo_sprite::combo_sprite(int combo_size, float x, float y, const gradient *g)
: x_origin(x)
, y_origin(y)
, y_offset(0)
, gradient_(g)
, ttl(TTL)
{
	const g2d::font *large_font = g2d::font_manager::get_instance().load("fonts/large");
	const g2d::font *small_font = g2d::font_manager::get_instance().load("fonts/small");

	wchar_t buf[80];
	xswprintf(buf, L"%d", combo_size);

	text_.text_program(get_program_instance<program_timer_text>().get_raw())
		.render_text(large_font, L"%d", combo_size)
		.translate(large_font->get_string_width(buf) + 12, 16)
		.render_text(small_font, L"chain!");
}

bool
combo_sprite::update(uint32_t dt)
{
	float dy = cosf(ttl*M_PI/TTL);
	dy *= dy;
	dy += .4;

	y_offset += dt*2.4*dy/MS_PER_TIC;

	return (ttl -= dt) > 0;
}

void
combo_sprite::draw(const g2d::mat4& proj_modelview) const
{
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
}
