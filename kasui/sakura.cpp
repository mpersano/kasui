#include <guava2d/texture_manager.h>

#include "render.h"

#include "common.h"
#include "sakura.h"

namespace {

constexpr int FADE_TTL = 30 * MS_PER_TIC;

constexpr int MIN_TTL = 150 * MS_PER_TIC;
constexpr int MAX_TTL = 300 * MS_PER_TIC;

constexpr float MIN_SIZE = 40;
constexpr float MAX_SIZE = 60;

constexpr float MIN_DELTA_ANGLE = .01;
constexpr float MAX_DELTA_ANGLE = .05;

constexpr float SPEED_FUZZ = .4;

constexpr float MIN_PHI = .025, MAX_PHI = .05;
constexpr float MIN_PHASE = 0, MAX_PHASE = M_PI;
constexpr float MIN_RADIUS = 30, MAX_RADIUS = 60;

constexpr float MIN_SPEED = 2.;
constexpr float MAX_SPEED = 4.;

constexpr float MIN_ALPHA = .3;
constexpr float MAX_ALPHA = .6;

} // anonymous namespace

void sakura_petal::reset(bool from_start)
{
    size = frand(MIN_SIZE, MAX_SIZE);

    pos.x = frand(.5 * window_width, 1.5 * window_width);
    pos.y = window_height + size;

    constexpr float f = 1.f / MS_PER_TIC;

    dir = g2d::vec2(-1, -2) + g2d::vec2(frand(-SPEED_FUZZ, SPEED_FUZZ), frand(-SPEED_FUZZ, SPEED_FUZZ));
    dir.set_length(f * frand(MIN_SPEED, MAX_SPEED));

    angle = 0;
    delta_angle = f * frand(MIN_DELTA_ANGLE, MAX_DELTA_ANGLE);

    phi_0 = f * frand(MIN_PHI, MAX_PHI);
    phi_1 = 2 * phi_0;

    phase_0 = frand(MIN_PHASE, MAX_PHASE);
    phase_1 = frand(MIN_PHASE, MAX_PHASE);

    radius_0 = frand(MIN_RADIUS, MAX_RADIUS);
    radius_1 = .5 * radius_0;

    alpha = frand(MIN_ALPHA, MAX_ALPHA);

    ttl = frand(MIN_TTL, MAX_TTL);

    if (from_start) {
        tics = 0;
    } else {
        tics = frand(0, ttl);
        pos += tics * dir;
        angle += tics * delta_angle;
    }
}

void sakura_petal::draw(const g2d::texture *texture) const
{
    const float a = cosf(phi_0 * tics + phase_0);
    const float b = cosf(phi_1 * tics + phase_1);

    const g2d::vec2 p = pos + g2d::vec2(a * radius_0, b * radius_1);

    const float s = sinf(angle);
    const float c = cosf(angle);

    const g2d::vec2 up = g2d::vec2(s, c) * .5 * size;
    const g2d::vec2 right = g2d::vec2(-c, s) * .5 * size;

    const g2d::vec2 p0 = p + up - right;
    const g2d::vec2 p1 = p + up + right;
    const g2d::vec2 p2 = p - up + right;
    const g2d::vec2 p3 = p - up - right;

    const float w = tics > ttl - FADE_TTL ? 1. - static_cast<float>(tics - (ttl - FADE_TTL)) / FADE_TTL : 1;

    render::set_color({1.f, 1.f, 1.f, w * alpha});

    render::draw_quad(texture, {{p0.x, p0.y}, {p1.x, p1.y}, {p2.x, p2.y}, {p3.x, p3.y}},
                      {{
                           0, 0,
                       },
                       {1, 0},
                       {1, 1},
                       {0, 1}},
                      -1);
}

void sakura_petal::update(uint32_t dt)
{
    pos += dt * dir;
    angle += dt * delta_angle;

    if ((tics += dt) >= ttl)
        reset(true);
}

sakura_fubuki::sakura_fubuki()
    : petal_texture_(g2d::texture_manager::get_instance().load("images/petal.png"))
{
}

void sakura_fubuki::update(uint32_t dt)
{
    for (auto &p : petals_)
        p.update(dt);
}

void sakura_fubuki::reset()
{
    for (auto &p : petals_)
        p.reset(false);
}

void sakura_fubuki::draw() const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    for (const auto &p : petals_)
        p.draw(petal_texture_);
}
