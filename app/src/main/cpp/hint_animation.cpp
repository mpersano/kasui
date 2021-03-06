#include "hint_animation.h"

#include <guava2d/texture_manager.h>
#include <guava2d/vec3.h>
#include <guava2d/xwchar.h>

#include "action.h"
#include "block_info.h"
#include "jukugo.h"
#include "settings.h" // for gradient
#include "tween.h"
#include "fonts.h"

hint_text_box::hint_text_box(const hint &h, float cell_size, float width, const gradient& g)
    : width_(width)
    , text_box_(width)
    , state_tics_(0)
    , tics_(0)
    , state_(state::INTRO)
    , gradient_(g)
    , block_texture_(g2d::load_texture("images/blocks.png"))
    , arrow_texture_(g2d::load_texture("images/arrow.png"))
{
    // text box

    wchar_t buf[1024];
    xswprintf(buf, L"@0%s【%s】\n@1%s", h.jukugo->reading, h.jukugo->kanji, h.jukugo->eigo);

    text_box_.set_text(buf);

    // block/arrow

    extern block_info block_infos[];
    const block_info &bi = block_infos[h.block_type & ~BAKUDAN_FLAG];
    const block_info::texuv &t = bi.texuvs[!!(h.block_type & BAKUDAN_FLAG)];

    const float sv = block_texture_->get_u_scale();
    const float su = block_texture_->get_v_scale();

    const g2d::vec2 uv0(su * t.u0, sv * t.v0);
    const g2d::vec2 uv1(su * t.u1, sv * t.v1);

    to_pos_ = cell_size * g2d::vec2(h.block_c, h.block_r);
    g2d::vec2 dir = cell_size * g2d::vec2(h.match_c, h.match_r) - to_pos_;
    from_pos_ = to_pos_ - .5 * dir;

    block_quad_ = {{0, 0}, {cell_size, cell_size}};
    block_texuv_ = {{uv0.y, uv1.x}, {uv1.y, uv0.x}};

    const g2d::vec2 u = .5 * cell_size * (to_pos_ - from_pos_).normalize();
    const g2d::vec2 v(-u.y, u.x);
    const g2d::vec2 p = to_pos_ + g2d::vec2(.5 * cell_size, .5 * cell_size);
    arrow_quad_ = {p - u - v, p - u + v, p + u + v, p + u - v};
}

hint_text_box::~hint_text_box()
{
}

void hint_text_box::draw() const
{
    const float scale = get_scale();
    const float alpha = get_alpha();

    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    // arrow

    render::set_color({1.f, 1.f, 1.f, .6f * alpha});
    render::draw_quad(arrow_texture_, arrow_quad_, {{0, 0}, {0, 1}, {1, 1}, {1, 0}}, 55);

    // block

    const int PERIOD = 10 * MS_PER_TIC;

    float t = fmodf(tics_, PERIOD) / PERIOD;
    const g2d::vec2 p = t * to_pos_ + (1. - t) * from_pos_;

    render::push_matrix();
    render::translate(p.x, p.y);
    render::set_color({1.f, 1.f, 1.f, (.5f + .5f * t * t) * alpha});
    render::draw_box(block_texture_, block_quad_, block_texuv_, 50);
    render::pop_matrix();

    // text box

    render::push_matrix();
    render::translate(pos_.x, pos_.y);
    render::scale(scale, scale);
    text_box_.draw(alpha);

    // title

    const g2d::rgb base_color = gradient_.to;
    const g2d::rgba text_color(base_color, alpha);
    const g2d::rgba outline_color(.5 * base_color, alpha);

    render::set_text_align(text_align::CENTER);
    render::set_color({1.f, 1.f, 1.f, alpha});
    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    render::draw_text(get_font(font::medium), {0, .5f * text_box_.get_height()}, 55, outline_color, text_color, L"hint!");

    render::pop_matrix();
}

bool hint_text_box::update(uint32_t dt)
{
    tics_ += dt;

    switch (state_) {
        case state::INTRO:
            if ((state_tics_ += dt) >= INTRO_TICS) {
                state_ = state::ACTIVE;
                state_tics_ = 0;
            }
            return true;

        case state::OUTRO:
            return (state_tics_ += dt) < OUTRO_TICS;

        default:
            return true;
    }
}

bool hint_text_box::close()
{
    if (state_ == state::ACTIVE) {
        state_ = state::OUTRO;
        state_tics_ = 0;
        return true;
    } else {
        return false;
    }
}

float hint_text_box::get_alpha() const
{
    switch (state_) {
        case state::INTRO:
            return static_cast<float>(state_tics_) / INTRO_TICS;

        case state::OUTRO:
            return 1. - static_cast<float>(state_tics_) / OUTRO_TICS;

        default:
            return 1;
    }
}

float hint_text_box::get_scale() const
{
    switch (state_) {
        case state::INTRO: {
            float t = static_cast<float>(state_tics_) / INTRO_TICS;
            return out_bounce_tween<float>()(.5, 1, t);
        }

        case state::OUTRO: {
            float t = static_cast<float>(state_tics_) / OUTRO_TICS;
            return in_back_tween<float>()(1, .5, t);
        }

        default:
            return 1.;
    }
}
