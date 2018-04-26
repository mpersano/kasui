#pragma once

#include "theme.h"

#include <array>

class flowers_theme : public theme_animation
{
public:
    flowers_theme();

    void reset() override;
    void draw() const override;
    void update(uint32_t dt) override;

private:
    struct flower
    {
        float size;
        g2d::vec2 pos, speed;
        float angle, delta_angle;
        int tics, ttl;

        void reset();
        void update(uint32_t dt);
    };

    std::array<flower, 16> flowers_;
    const g2d::texture *texture_;
};
