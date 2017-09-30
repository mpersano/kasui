#include <guava2d/vec3.h>
#include <guava2d/font_manager.h>
#include <guava2d/texture_manager.h>
#include <guava2d/xwchar.h>

#include "action.h"
#include "tween.h"
#include "jukugo.h"
#include "program_registry.h"
#include "settings.h" // for gradient
#include "block_info.h"
#include "hint_animation.h"

hint_text_box::hint_text_box(const hint& h, float cell_size, float width, const gradient *g)
: width_(width)
, text_box_(width)
, state_tics_(0)
, tics_(0)
, state_(INTRO)
, gradient_(g)
, block_texture_(g2d::texture_manager::get_instance().load("images/blocks.png"))
, arrow_texture_(g2d::texture_manager::get_instance().load("images/arrow.png"))
{
	// text box

	wchar_t buf[1024];
	xswprintf(buf, L"@0%s【%s】\n@1%s", h.jukugo->reading, h.jukugo->kanji, h.jukugo->eigo);

	text_box_.set_text(buf);

	// title

	const wchar_t *TITLE = L"hint!";

	const g2d::font *font = g2d::font_manager::get_instance().load("fonts/medium");

	title_texture_ = font->get_texture();

	float x = -.5*font->get_string_width(TITLE);

	for (const wchar_t *p = TITLE; *p; p++) {
		const g2d::glyph_info *gi = font->find_glyph(*p);

		const float x_left = x + gi->left;
		const float x_right = x_left + gi->width;
		const float y_top = gi->top;
		const float y_bottom = y_top - gi->height;

		static g2d::vertex_array_texuv gv(4);

		title_va_ << x_left, y_top, gi->texuv[0].x, gi->texuv[0].y;
		title_va_ << x_right, y_top, gi->texuv[1].x, gi->texuv[1].y;
		title_va_ << x_right, y_bottom, gi->texuv[2].x, gi->texuv[2].y;

		title_va_ << x_right, y_bottom, gi->texuv[2].x, gi->texuv[2].y;
		title_va_ << x_left, y_bottom, gi->texuv[3].x, gi->texuv[3].y;
		title_va_ << x_left, y_top, gi->texuv[0].x, gi->texuv[0].y;

		x += gi->advance_x;
	}

	// block/arrow

	extern block_info block_infos[];

	const block_info& bi = block_infos[h.block_type & ~BAKUDAN_FLAG];
	const block_info::texuv& t = bi.texuvs[!!(h.block_type & BAKUDAN_FLAG)];

	const float sv = block_texture_->get_u_scale();
	const float su = block_texture_->get_v_scale();

	const g2d::vec2 uv0(su*t.u0, sv*t.v0);
	const g2d::vec2 uv1(su*t.u1, sv*t.v1);

	to_pos_ = cell_size*g2d::vec2(h.block_c, h.block_r);
	g2d::vec2 dir = cell_size*g2d::vec2(h.match_c, h.match_r) - to_pos_;
	from_pos_ = to_pos_ - .5*dir;

	block_va_ << 0, 0, uv0.y, uv1.x;
	block_va_ << cell_size, 0, uv1.y, uv1.x;

	block_va_ << 0, cell_size, uv0.y, uv0.x;
	block_va_ << cell_size, cell_size, uv1.y, uv0.x;

	g2d::vec2 u = .5*cell_size*(to_pos_ - from_pos_).normalize();
	g2d::vec2 v(-u.y, u.x);

	g2d::vec2 p = to_pos_ + g2d::vec2(.5*cell_size, .5*cell_size);

	g2d::vec2 p0 = p - u - v;
	g2d::vec2 p1 = p - u + v;
	g2d::vec2 p2 = p + u - v;
	g2d::vec2 p3 = p + u + v;

	arrow_va_ << p0.x, p0.y, 0, 0;
	arrow_va_ << p1.x, p1.y, 0, 1;

	arrow_va_ << p2.x, p2.y, 1, 0;
	arrow_va_ << p3.x, p3.y, 1, 1;
}

hint_text_box::~hint_text_box()
{ }

void
hint_text_box::draw() const
{
#ifdef FIX_ME
	float scale;

	switch (state_) {
		case INTRO:
			{
			float t = static_cast<float>(state_tics_)/INTRO_TICS;
			scale = out_bounce_tween<float>()(.5, 1, t);
			}
			break;

		case OUTRO:
			{
			float t = static_cast<float>(state_tics_)/OUTRO_TICS;
			scale = in_back_tween<float>()(1, .5, t);
			}
			break;

		default:
			scale = 1.;
			break;
	}

	const float alpha = get_alpha();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	auto& prog = get_program_instance<program_texture_uniform_alpha>();
	prog.use();
	prog.set_texture(0);

	// arrow

	prog.set_proj_modelview_matrix(mat);
	prog.set_alpha(.6*alpha);

	arrow_texture_->bind();
	arrow_va_.draw(GL_TRIANGLE_STRIP);

	// block

	const int PERIOD = 10*MS_PER_TIC;

	float t = fmodf(tics_, PERIOD)/PERIOD;
	const g2d::vec2 p = t*to_pos_ + (1. - t)*from_pos_;

	prog.set_proj_modelview_matrix(mat*g2d::mat4::translation(p.x, p.y, 0));
	prog.set_alpha((.5 + .5*t*t)*alpha);

	block_texture_->bind();
	block_va_.draw(GL_TRIANGLE_STRIP);

	// text box

	g2d::mat4 m = mat*g2d::mat4::translation(pos_.x, pos_.y, 0)*g2d::mat4::scale(scale, scale, 0);
	text_box_.draw(m, alpha);

	// title

	{
	auto& prog = get_program_instance<program_timer_text>();
	prog.use();
	prog.set_proj_modelview_matrix(m*g2d::mat4::translation(0, .5*text_box_.get_height(), 0));
	prog.set_texture(0);

	const g2d::rgb& text_color = *gradient_->to*(1./255.);
	const g2d::rgb& outline_color = .5*text_color;

	prog.set_text_color(g2d::rgba(text_color, alpha));
	prog.set_outline_color(g2d::rgba(outline_color, alpha));
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	title_texture_->bind();

	title_va_.draw(GL_TRIANGLES);
#endif
}

bool
hint_text_box::update(uint32_t dt)
{
	tics_ += dt;

	switch (state_) {
		case INTRO:
			if ((state_tics_ += dt) >= INTRO_TICS) {
				state_ = ACTIVE;
				state_tics_ = 0;
			}
			return true;

		case OUTRO:
			return (state_tics_ += dt) < OUTRO_TICS;

		default:
			return true;
	}
}

bool
hint_text_box::close()
{
	if (state_ == ACTIVE) {
		state_ = OUTRO;
		state_tics_ = 0;
		return true;
	} else {
		return false;
	}
}

float
hint_text_box::get_alpha() const
{
	switch (state_) {
		case INTRO:
			return static_cast<float>(state_tics_)/INTRO_TICS;

		case OUTRO:
			return 1. - static_cast<float>(state_tics_)/OUTRO_TICS;

		default:
			return 1;
	}
}
