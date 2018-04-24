#include "clouds_theme.h"

#include "guava2d/texture_manager.h"

#include "common.h"
#include "render.h"

#include <algorithm>

namespace {
constexpr int CLOUD_WIDTH = 256;
constexpr int CLOUD_HEIGHT = 256;
constexpr int NUM_CLOUD_TYPES = 4;
}

void clouds_theme::cloud::reset()
{
    depth = frand(.8, 2.);
    scale = 1.5 / depth;

    // pos.x = (-.5*window_width - scale*CLOUD_WIDTH)*frand(1, 2);
    pos.x = frand(-.5 * window_width - scale * CLOUD_WIDTH, .5 * window_width);
    pos.y = frand(-.25 * window_width, .5 * window_height + .8 * scale * CLOUD_HEIGHT);

    type = rand() % NUM_CLOUD_TYPES;
}

void clouds_theme::cloud::update(uint32_t dt)
{
    const float f = 1.f / MS_PER_TIC;

    const float speed = dt * f * .8 / depth;

    if ((pos.x += speed) >= .5 * window_width) {
        reset();
        pos.x = -.5 * window_width - scale * CLOUD_WIDTH;
    }
}

clouds_theme::clouds_theme()
    : texture_{g2d::texture_manager::get_instance().load("images/clouds.png")}
{
    reset();
}

void clouds_theme::reset()
{
    for (auto& cloud : clouds_)
        cloud.reset();
}

void clouds_theme::draw() const
{
    std::array<const cloud *, NUM_CLOUDS> sorted_clouds;
    for (int i = 0; i < NUM_CLOUDS; i++)
        sorted_clouds[i] = &clouds_[i];
    std::sort(std::begin(sorted_clouds), std::end(sorted_clouds), [](const cloud *a, const cloud *b) {
        return a->depth > b->depth;
    });

    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    for (auto p : sorted_clouds) {
        const float w = p->scale * CLOUD_WIDTH;
        const float h = p->scale * CLOUD_HEIGHT;

        const float du = texture_->get_u_scale() / NUM_CLOUD_TYPES;
        const float u = du * p->type;

        const float dv = texture_->get_v_scale();

        const auto& pos = p->pos + .5 * g2d::vec2{window_width, window_height};
        render::draw_box(texture_, {pos, pos + g2d::vec2{w, -h}}, {g2d::vec2{u, 0}, g2d::vec2{u + du, dv}}, -99);
    }
}

void clouds_theme::update(uint32_t dt)
{
    for (auto& cloud : clouds_)
        cloud.update(dt);
}
