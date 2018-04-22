#include <stdlib.h>

#include "guava2d/g2dgl.h"
#include "guava2d/texture_manager.h"
#include "guava2d/vertex_array.h"

#include "gl_check.h"
#include "clouds_theme.h"
#include "common.h"
#include "program_registry.h"
#include "render.h"

enum
{
    NUM_CLOUDS = 16,
    CLOUD_WIDTH = 256,
    CLOUD_HEIGHT = 256,
    NUM_CLOUD_TYPES = 4,
};

struct cloud
{
    using vertex_array_type = g2d::indexed_vertex_array<GLubyte, g2d::vertex::attrib<GLfloat, 2>, g2d::vertex::attrib<GLfloat, 2>>;

    float depth;
    g2d::vec2 pos;
    float scale;
    int type;

    void reset();
    void draw(vertex_array_type &gv) const;
    void update(uint32_t dt);
};

static cloud clouds[NUM_CLOUDS];
static const g2d::texture *clouds_texture;

void cloud::reset()
{
    depth = frand(.8, 2.);
    scale = 1.5 / depth;

    // pos.x = (-.5*window_width - scale*CLOUD_WIDTH)*frand(1, 2);
    pos.x = frand(-.5 * window_width - scale * CLOUD_WIDTH, .5 * window_width);
    pos.y = frand(-.25 * window_width, .5 * window_height + .8 * scale * CLOUD_HEIGHT);

    type = rand() % NUM_CLOUD_TYPES;
}

void cloud::draw(vertex_array_type &gv) const
{
    const float w = scale * CLOUD_WIDTH;
    const float h = scale * CLOUD_HEIGHT;

    const float du = clouds_texture->get_u_scale() / NUM_CLOUD_TYPES;
    const float u = du * type;

    const float dv = clouds_texture->get_v_scale();

    const int vert_index = gv.get_num_verts();

    gv << pos.x, pos.y, u, 0;
    gv << pos.x + w, pos.y, u + du, 0;
    gv << pos.x + w, pos.y - h, u + du, dv;
    gv << pos.x, pos.y - h, u, dv;

    gv < vert_index + 0, vert_index + 1, vert_index + 2;
    gv < vert_index + 2, vert_index + 3, vert_index + 0;
}

void cloud::update(uint32_t dt)
{
    const float f = 1.f / MS_PER_TIC;

    const float speed = dt * f * .8 / depth;

    if ((pos.x += speed) >= .5 * window_width) {
        reset();
        pos.x = -.5 * window_width - scale * CLOUD_WIDTH;
    }
}

static int cloud_depth_compare(const void *foo, const void *bar)
{
    const float d0 = (*(cloud **)foo)->depth;
    const float d1 = (*(cloud **)bar)->depth;
    return d0 > d1 ? -1 : 1;
}

clouds_theme::clouds_theme()
{
    clouds_texture = g2d::texture_manager::get_instance().load("images/clouds.png");
}

void clouds_theme::reset()
{
    for (auto& cloud : clouds)
        cloud.reset();
}

void clouds_theme::draw() const
{
    render::end_batch();

    // HACK
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CHECK(glBindVertexArray(0));

    static cloud::vertex_array_type gv(NUM_CLOUDS * 4, NUM_CLOUDS * 6);

    cloud *sorted_clouds[NUM_CLOUDS];

    for (int i = 0; i < NUM_CLOUDS; i++)
        sorted_clouds[i] = &clouds[i];

    qsort(sorted_clouds, NUM_CLOUDS, sizeof *sorted_clouds, cloud_depth_compare);

    gv.reset();
    for (auto cloud : sorted_clouds)
        cloud->draw(gv);

    const g2d::mat4 proj_modelview =
        get_ortho_projection() * g2d::mat4::translation(.5 * window_width, .5 * window_height, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    clouds_texture->bind();

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
    for (auto& cloud : clouds)
        cloud.update(dt);
}
