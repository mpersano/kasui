#include "combo_sprite.h"

#include "common.h"
#include "render.h"
#include "settings.h"
#include "fonts.h"

#include <sstream>

combo_sprite::combo_sprite(int combo_size, float x, float y, const gradient& g)
    : x_origin_(x)
    , y_origin_(y)
    , gradient_(g)
{
    // XXX use boost::lexical_cast here
    std::wstringstream ss;
    ss << combo_size;
    combo_size_ = ss.str();

    x_chain_text_ = get_font(font::large)->get_string_width(combo_size_.c_str()) + 12;
}

bool combo_sprite::update(uint32_t dt)
{
    float dy = cosf(ttl_ * M_PI / TTL);
    dy *= dy;
    dy += .4;

    y_offset_ += dt * 2.4 * dy / MS_PER_TIC;

    return (ttl_ -= dt) > 0;
}

void combo_sprite::draw() const
{
    float alpha = sinf(ttl_ * M_PI / TTL);
    alpha *= alpha;

    const g2d::rgb base_color = gradient_.to;
    const g2d::rgba text_color(base_color, alpha);
    const g2d::rgba outline_color(.5 * base_color, alpha);

    render::set_blend_mode(blend_mode::ALPHA_BLEND);
    render::set_color({1.f, 1.f, 1.f, alpha});

    render::push_matrix();
    render::translate(x_origin_, y_origin_ + y_offset_);

    render::draw_text(get_font(font::large), {}, 50, outline_color, text_color, combo_size_.c_str());
    render::draw_text(get_font(font::small), {x_chain_text_, 16}, 50, outline_color, text_color, L"chain!");

    render::pop_matrix();
}
