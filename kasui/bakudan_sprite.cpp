#include "bakudan_sprite.h"

#include "guava2d/rgb.h"
#include "guava2d/texture_manager.h"

#include "common.h"
#include "render.h"

#include <cassert>
#include <cmath>

static constexpr int TTL = 70 * MS_PER_TIC;
static constexpr int GLOW_TEXTURE_SIZE = 64;

bakudan_sprite::bakudan_sprite(float x, float y)
    : x_center(x)
    , y_center(y)
    , start_angle(frand(0., M_PI))
    , tics(0)
    , glow_texture(g2d::load_texture("images/glow.png"))
{
}

bakudan_sprite::~bakudan_sprite()
{
}

bool bakudan_sprite::update(uint32_t dt)
{
    return (tics += dt) < TTL;
}

void bakudan_sprite::draw() const
{
    constexpr int NUM_RAYS = 9;

    constexpr float MIN_RADIUS = 100;
    constexpr float MAX_RADIUS = 600;

    const float lerp_factor = sinf(static_cast<float>(tics) * M_PI / TTL);

    const float radius_rays = MIN_RADIUS + lerp_factor * (MAX_RADIUS - MIN_RADIUS);
    const float radius_glow = .6 * radius_rays;

    float angle = start_angle + .01 * tics / MS_PER_TIC + .6 * lerp_factor;

    const float delta_angle = 2. * M_PI / NUM_RAYS;
    const float fray = .6 - .4 * lerp_factor;

    const float ray_alpha = powf(lerp_factor, 5);

    render::set_blend_mode(blend_mode::ADDITIVE_BLEND);

    const g2d::rgba center_color{ray_alpha, ray_alpha, ray_alpha, ray_alpha};
    const g2d::rgba black{0, 0, 0, 0};

    for (int i = 0; i < NUM_RAYS; i++) {
        const float x0 = x_center + cosf(angle) * radius_rays;
        const float y0 = y_center + sinf(angle) * radius_rays;

        const float x1 = x_center + cosf(angle + fray * delta_angle) * radius_rays;
        const float y1 = y_center + sinf(angle + fray * delta_angle) * radius_rays;

        render::draw_quad(
                {{x_center, y_center}, {x0, y0}, {x1, y1}, {x_center, y_center}},
                {center_color, black, black, center_color},
                48);

        angle += delta_angle;
    }


    const float x0 = x_center - radius_glow;
    const float x1 = x_center + radius_glow;

    const float y0 = y_center - radius_glow;
    const float y1 = y_center + radius_glow;

    const float glow_alpha = lerp_factor;

    const float du = glow_texture->get_u_scale();
    const float dv = glow_texture->get_v_scale();

    render::set_color({1, 1, 1, glow_alpha});
    render::draw_quad(
            glow_texture,
            {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}},
            {{0, 0}, {du, 0}, {du, dv}, {0, dv}},
            48);
}
