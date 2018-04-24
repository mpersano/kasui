#include "clouds_theme.h"

#include "guava2d/g2dgl.h"
#include "guava2d/texture_manager.h"
#include "guava2d/vertex_array.h"

#include "gl_check.h"
#include "common.h"
#include "program_registry.h"
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
    render::end_batch();

    // HACK
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CHECK(glBindVertexArray(0));

    using vertex_array_type = g2d::indexed_vertex_array<GLubyte,
                                                        g2d::vertex::attrib<GLfloat, 2>,
                                                        g2d::vertex::attrib<GLfloat, 2>>;

    static vertex_array_type gv(NUM_CLOUDS * 4, NUM_CLOUDS * 6);

    const cloud *sorted_clouds[NUM_CLOUDS];
    for (int i = 0; i < NUM_CLOUDS; i++)
        sorted_clouds[i] = &clouds_[i];

    std::sort(std::begin(sorted_clouds), std::end(sorted_clouds), [](const cloud *a, const cloud *b) {
        return a->depth > b->depth;
    });

    gv.reset();
    for (auto cloud : sorted_clouds) {
        const float w = cloud->scale * CLOUD_WIDTH;
        const float h = cloud->scale * CLOUD_HEIGHT;

        const float du = texture_->get_u_scale() / NUM_CLOUD_TYPES;
        const float u = du * cloud->type;

        const float dv = texture_->get_v_scale();

        const int vert_index = gv.get_num_verts();

        const auto& pos = cloud->pos;
        gv << pos.x, pos.y, u, 0;
        gv << pos.x + w, pos.y, u + du, 0;
        gv << pos.x + w, pos.y - h, u + du, dv;
        gv << pos.x, pos.y - h, u, dv;

        gv < vert_index + 0, vert_index + 1, vert_index + 2;
        gv < vert_index + 2, vert_index + 3, vert_index + 0;
    }

    const g2d::mat4 proj_modelview =
        get_ortho_projection() * g2d::mat4::translation(.5 * window_width, .5 * window_height, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    texture_->bind();

    program_texture_decal &prog = get_program_instance<program_texture_decal>();
    prog.use();
    prog.set_proj_modelview_matrix(proj_modelview);
    prog.set_texture(0);

    gv.draw(GL_TRIANGLES);

    render::begin_batch();
    render::set_viewport(0, window_width, 0, window_height);
}

void clouds_theme::update(uint32_t dt)
{
    for (auto& cloud : clouds_)
        cloud.update(dt);
}
