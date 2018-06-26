#pragma once

#include <guava2d/vec2.h>

#include <array>

namespace g2d {
class texture;
}

struct sakura_petal
{
    float size;
    g2d::vec2 pos, dir;
    float angle, delta_angle;
    int tics, ttl;
    float phase_0, phi_0, radius_0;
    float phase_1, phi_1, radius_1;
    float alpha;

    void reset(bool);
    void draw(const g2d::texture *texture) const;
    void update(uint32_t dt);
};

class sakura_fubuki
{
public:
    sakura_fubuki();

    void reset();
    void draw() const;
    void update(uint32_t dt);

private:
    static constexpr int NUM_PETALS = 20;
    std::array<sakura_petal, NUM_PETALS> petals_;

    const g2d::texture *petal_texture_;
};
