#pragma once

#include "theme.h"

#include "guava2d/vec3.h"
#include "guava2d/rgb.h"

#include <array>

namespace g2d
{
class texture;
class program;
};

class falling_leaves_theme : public theme_animation
{
public:
    falling_leaves_theme();

    void reset() override;
    void draw() const override;
    void update(uint32_t dt) override;

private:
    struct leaf
    {
#ifdef FIX_ME
        using vertex_array = g2d::indexed_vertex_array<GLubyte, g2d::vertex::attrib<GLfloat, 3>, g2d::vertex::attrib<GLshort, 2>, g2d::vertex::attrib<GLubyte, 4>>;
#endif

        float size;
        g2d::vec3 pos, speed;
        g2d::rgb color;
        g2d::mat4 dir;
        g2d::vec3 axis;
        float rot;
        float phase_0, phase_1, phi_0, phi_1;
        float radius_0, radius_1;
        int tics, ttl;

        void reset();
#ifdef FIX_ME
        void draw(vertex_array &gv) const;
#endif
        void update(uint32_t dt);
    };

    static constexpr int NUM_LEAVES = 30;
    std::array<leaf, NUM_LEAVES> leaves_;

    const g2d::texture *texture_;
    const g2d::program *program_;
};
