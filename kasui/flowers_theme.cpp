#include <stdlib.h>

#include "guava2d/g2dgl.h"
#include "guava2d/texture_manager.h"
#include "guava2d/vec2.h"
#include "guava2d/vertex_array.h"

#include "common.h"
#include "flowers_theme.h"
#include "program_registry.h"

enum
{
    NUM_FLOWERS = 16,
    FADE_TTL = 30 * MS_PER_TIC,
};

struct flower
{
    typedef g2d::indexed_vertex_array<GLubyte, g2d::vertex::attrib<GLfloat, 2>, g2d::vertex::attrib<GLshort, 2>,
                                      g2d::vertex::attrib<GLfloat, 1>>
        vertex_array_type;

    float size;
    g2d::vec2 pos, speed;
    float angle, delta_angle;
    int tics, ttl;

    void reset();
    void draw(vertex_array_type &gv) const;
    void update(uint32_t dt);
};

static flower flowers[NUM_FLOWERS];
static const g2d::texture *ume_texture;

void flower::reset()
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

void flower::draw(vertex_array_type &gv) const
{
    float alpha_scale;

    if (tics < FADE_TTL)
        alpha_scale = static_cast<float>(tics) / FADE_TTL;
    else if (tics > ttl - FADE_TTL)
        alpha_scale = 1. - static_cast<float>(tics - (ttl - FADE_TTL)) / FADE_TTL;
    else
        alpha_scale = 1;

    const float a = alpha_scale * .6;

    const float cur_size = size * powf(1.005, static_cast<float>(tics) / MS_PER_TIC);
    const float c = cosf(angle) * cur_size;
    const float s = sinf(angle) * cur_size;

    const g2d::vec2 up = g2d::vec2(c, s);
    const g2d::vec2 side = g2d::vec2(-s, c);

    const g2d::vec2 p0 = pos + up + side;
    const g2d::vec2 p1 = pos + up - side;
    const g2d::vec2 p2 = pos - up - side;
    const g2d::vec2 p3 = pos - up + side;

    const int vert_index = gv.get_num_verts();

    gv << p0.x, p0.y, 0, 0, a;
    gv << p1.x, p1.y, 1, 0, a;
    gv << p2.x, p2.y, 1, 1, a;
    gv << p3.x, p3.y, 0, 1, a;

    gv < vert_index + 0, vert_index + 1, vert_index + 2;
    gv < vert_index + 2, vert_index + 3, vert_index + 0;
}

void flower::update(uint32_t dt)
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

static void initialize()
{
    ume_texture = g2d::texture_manager::get_instance().load("images/ume.png");
}

static void reset()
{
    for (flower *p = flowers; p != &flowers[NUM_FLOWERS]; p++)
        p->reset();
}

static void update(uint32_t dt)
{
    for (flower *p = flowers; p != &flowers[NUM_FLOWERS]; p++)
        p->update(dt);
}

static int flower_z_compare(const void *foo, const void *bar)
{
    const float s0 = (*(flower **)foo)->size;
    const float s1 = (*(flower **)bar)->size;
    return s0 < s1 ? -1 : 1;
}

static void draw()
{
    static flower *sorted_flowers[NUM_FLOWERS];

    for (int i = 0; i < NUM_FLOWERS; i++)
        sorted_flowers[i] = &flowers[i];

    qsort(sorted_flowers, NUM_FLOWERS, sizeof *sorted_flowers, flower_z_compare);

    static flower::vertex_array_type gv(NUM_FLOWERS * 4, NUM_FLOWERS * 6);

    gv.reset();

    for (auto &sorted_flower : sorted_flowers)
        sorted_flower->draw(gv);

    const g2d::mat4 proj_modelview =
        get_ortho_projection() * g2d::mat4::translation(.5 * window_width, .5 * window_height, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ume_texture->bind();

    program_texture_alpha &prog = get_program_instance<program_texture_alpha>();
    prog.use();
    prog.set_proj_modelview_matrix(proj_modelview);
    prog.set_texture(0);

    gv.draw(GL_TRIANGLES);
}

theme flowers_theme = {initialize, reset, draw, update};
