#include <algorithm>

#include "guava2d/g2dgl.h"
#include "guava2d/texture_manager.h"
#include "guava2d/vec2.h"

#include "common.h"
#include "flowers_theme.h"
#include "render.h"

constexpr auto FADE_TTL = 30 * MS_PER_TIC;

void flowers_theme::flower::reset()
{
    const float f = 1.f / MS_PER_TIC;

    size = frand(32, 40);

    pos = g2d::vec2(frand(-.4 * window_width, .4 * window_width), frand(-.4 * window_height, .4 * window_height));
    speed = f * pos * .0025;

    angle = frand(0., M_PI);
    delta_angle = f * frand(.005, .015);

    if (rand() % 2)
        delta_angle = -delta_angle;

    tics = 0;
    ttl = frand(2 * FADE_TTL, 150 * MS_PER_TIC);
}

void flowers_theme::flower::update(uint32_t dt)
{
    const g2d::vec2 cur_speed = speed * powf(1.005, static_cast<float>(tics) / MS_PER_TIC);

    pos += dt * cur_speed;
    angle += dt * delta_angle;

#if 0
	const float delta_size = 1.005;

	size *= dt*delta_size;
	speed *= dt*delta_size;
#endif

    if ((tics += dt) >= ttl)
        reset();
}

flowers_theme::flowers_theme()
    : texture_{g2d::texture_manager::get_instance().load("images/ume.png")}
{
    reset();
}

void flowers_theme::reset()
{
    for (auto& flower : flowers_)
        flower.reset();
}

void flowers_theme::update(uint32_t dt)
{
    for (auto& flower : flowers_)
        flower.update(dt);
}

void flowers_theme::draw() const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    for (const auto& p : flowers_) {
        const auto tics = p.tics;
        const auto ttl = p.ttl;

        float alpha_scale;
        if (tics < FADE_TTL)
            alpha_scale = static_cast<float>(tics) / FADE_TTL;
        else if (tics > ttl - FADE_TTL)
            alpha_scale = 1. - static_cast<float>(tics - (ttl - FADE_TTL)) / FADE_TTL;
        else
            alpha_scale = 1;

        const float a = alpha_scale * .6;

        const float cur_size = p.size * powf(1.005, static_cast<float>(tics) / MS_PER_TIC);
        const float c = cosf(p.angle) * cur_size;
        const float s = sinf(p.angle) * cur_size;

        const g2d::vec2 up = g2d::vec2(c, s);
        const g2d::vec2 side = g2d::vec2(-s, c);

        const auto& pos = p.pos + .5 * g2d::vec2{window_width, window_height};
        const g2d::vec2 p0 = pos + up + side;
        const g2d::vec2 p1 = pos + up - side;
        const g2d::vec2 p2 = pos - up - side;
        const g2d::vec2 p3 = pos - up + side;

        render::set_color({1, 1, 1, a});
        render::draw_quad(texture_,
                {p0, p1, p2, p3},
                {{0, 0}, {1, 0}, {1, 1}, {0, 1}},
                -99);
    }
}
