#pragma once

#include "theme.h"

#include <guava2d/vec2.h>

#include <array>

namespace g2d {
class texture;
}

class clouds_theme : public theme_animation
{
public:
    clouds_theme();

    void reset() override;
    void draw() const override;
    void update(uint32_t dt) override;

    struct cloud
    {
        float depth;
        g2d::vec2 pos;
        float scale;
        int type;

        void reset();
        void update(uint32_t dt);
    };

private:
    const g2d::texture *texture_;

    static constexpr int NUM_CLOUDS = 16;
    std::array<cloud, NUM_CLOUDS> clouds_;
};
