#include "render.h"
#include "noncopyable.h"

#include "gl_check.h"
#include "program_manager.h"

#include <guava2d/texture.h>
#include <guava2d/program.h>
#include <guava2d/rgb.h>
#include <guava2d/sprite.h> // XXX remove this eventually

#include <cassert>
#include <algorithm>
#include <stack>

namespace {

void gl_set_blend_mode(blend_mode mode)
{
    switch (mode) {
        case blend_mode::NO_BLEND:
            GL_CHECK(glDisable(GL_BLEND));
            break;

        case blend_mode::ALPHA_BLEND:
            GL_CHECK(glEnable(GL_BLEND));
            GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
            break;

        case blend_mode::ADDITIVE_BLEND:
            GL_CHECK(glEnable(GL_BLEND));
            GL_CHECK(glBlendFunc(GL_ONE, GL_ONE));
            break;
    }
}

class sprite_batch : private noncopyable
{
public:
    sprite_batch();

    void init();

    void set_viewport(int x_min, int x_max, int y_min, int y_max);

    void begin_batch();
    void end_batch();

    void push_matrix();
    void pop_matrix();

    void translate(float x, float y);
    void scale(float sx, float sy);
    void rotate(float a);

    void set_blend_mode(blend_mode mode);
    void set_color(const g2d::rgba& color);

    void add_quad(const g2d::program *program, const g2d::texture *texture, const quad& verts, const quad& texcoords, int layer);
    void add_quad(const g2d::texture *texture, const quad& verts, const quad& texcoords, int layer);
    void add_quad(const quad& verts, int layer);

private:
    struct sprite
    {
        int layer;
        const g2d::program *program;
        const g2d::texture *texture;
        quad verts;
        quad texcoords;
        blend_mode blend;
        g2d::rgba color;
    };

    void load_programs();

    void flush_queue();
    void render_sprites_texture(const sprite *const *sprites, int num_sprites) const;
    void render_sprites_flat(const sprite *const *sprites, int num_sprites) const;

    static const int SPRITE_QUEUE_CAPACITY = 1024;

    int sprite_queue_size_;
    sprite sprite_queue_[SPRITE_QUEUE_CAPACITY];

    blend_mode blend_mode_;
    g2d::rgba color_;
    g2d::mat3 matrix_;
    std::stack<g2d::mat3> matrix_stack_;

    const g2d::program *program_texture_;
    const g2d::program *program_flat_;

    std::array<GLfloat, 16> proj_matrix_;
} g_sprite_batch;

sprite_batch::sprite_batch()
{
}

void sprite_batch::init()
{
    load_programs();
}

void sprite_batch::set_viewport(int x_min, int x_max, int y_min, int y_max)
{
    const float a = 2.f/(x_max - x_min);
    const float b = 2.f/(y_max - y_min);

    const float tx = -(x_max + x_min)/(x_max - x_min);
    const float ty = -(y_max + y_min)/(y_max - y_min);

    proj_matrix_ = {  a,  0, 0, 0,
                      0,  b, 0, 0,
                      0,  0, 0, 0,
                     tx, ty, 0, 1 };

    program_texture_->use();
    program_texture_->set_uniform_matrix4("proj_modelview", &proj_matrix_[0]);
    program_texture_->set_uniform_i("tex", 0);

    program_flat_->use();
    program_flat_->set_uniform_matrix4("proj_modelview", &proj_matrix_[0]);
}

void sprite_batch::begin_batch()
{
    sprite_queue_size_ = 0;
    blend_mode_ = blend_mode::NO_BLEND;
    color_ = { 1, 1, 1, 1 };
    matrix_ = g2d::mat3::identity();
    matrix_stack_ = std::stack<g2d::mat3>();
}

void sprite_batch::end_batch()
{
    flush_queue();
}

void sprite_batch::push_matrix()
{
    matrix_stack_.push(matrix_);
}

void sprite_batch::pop_matrix()
{
    assert(!matrix_stack_.empty());
    matrix_ = matrix_stack_.top();
    matrix_stack_.pop();
}

void sprite_batch::translate(float x, float y)
{
    matrix_ *= g2d::mat3::translation(x, y);
}

void sprite_batch::scale(float sx, float sy)
{
    matrix_ *= g2d::mat3::scale(sx, sy);
}

void sprite_batch::rotate(float a)
{
    matrix_ *= g2d::mat3::rotation(a);
}

void sprite_batch::set_blend_mode(blend_mode mode)
{
    blend_mode_ = mode;
}

void sprite_batch::set_color(const g2d::rgba& color)
{
    color_ = color;
}

void sprite_batch::add_quad(const g2d::program *program, const g2d::texture *texture, const quad& verts, const quad& texcoords, int layer)
{
    if (sprite_queue_size_ == SPRITE_QUEUE_CAPACITY)
        flush_queue();

    auto& s = sprite_queue_[sprite_queue_size_++];

    s.program = program;
    s.texture = texture;

    s.verts.v00 = matrix_*verts.v00;
    s.verts.v01 = matrix_*verts.v01;
    s.verts.v10 = matrix_*verts.v10;
    s.verts.v11 = matrix_*verts.v11;

    s.texcoords = texcoords;

    s.layer = layer;
    s.blend = blend_mode_;
    s.color = color_;
}

void sprite_batch::add_quad(const g2d::texture *texture, const quad& verts, const quad& texcoords, int layer)
{
    add_quad(nullptr, texture, verts, texcoords, layer);
}

void sprite_batch::add_quad(const quad& verts, int layer)
{
    if (sprite_queue_size_ == SPRITE_QUEUE_CAPACITY)
        flush_queue();

    auto& s = sprite_queue_[sprite_queue_size_++];

    s.texture = nullptr;

    s.verts.v00 = matrix_*verts.v00;
    s.verts.v01 = matrix_*verts.v01;
    s.verts.v10 = matrix_*verts.v10;
    s.verts.v11 = matrix_*verts.v11;

    s.layer = layer;
    s.blend = blend_mode_;
    s.color = color_;
}

void sprite_batch::load_programs()
{
    program_texture_ = load_program("shaders/sprite.vert", "shaders/sprite.frag");
    program_flat_ = load_program("shaders/flat.vert", "shaders/flat.frag");
}

void sprite_batch::flush_queue()
{
    if (sprite_queue_size_ == 0)
        return;

    static const sprite *sorted_sprites[SPRITE_QUEUE_CAPACITY];

    for (int i = 0; i < sprite_queue_size_; ++i)
        sorted_sprites[i] = &sprite_queue_[i];

    std::stable_sort(&sorted_sprites[0], &sorted_sprites[sprite_queue_size_], [](const sprite *s0, const sprite *s1) {
        if (s0->layer != s1->layer) {
            return s0->layer < s1->layer;
        } else if (s0->blend != s1->blend) {
            return static_cast<int>(s0->blend) < static_cast<int>(s1->blend);
        } else if (s0->program != s1->program) {
            return s0->program < s1->program;
        } else {
            return s0->texture < s1->texture;
        }
    });

    const auto bind_texture = [this](const g2d::program *program, const g2d::texture *texture) {
        if (texture)
            texture->bind();

        if (program != nullptr) {
            program->use();
            program->set_uniform_matrix4("proj_modelview", &proj_matrix_[0]);
            if (texture)
                program->set_uniform_i("tex", 0);
        } else {
            if (texture != nullptr)
                program_texture_->use();
            else
                program_flat_->use();
        }
    };

    auto cur_program = sorted_sprites[0]->program;
    auto cur_texture = sorted_sprites[0]->texture;
    auto cur_blend_mode = sorted_sprites[0]->blend;

    bind_texture(cur_program, cur_texture);
    gl_set_blend_mode(cur_blend_mode);

    int batch_start = 0;

    const auto render_sprites = [&](int batch_end) {
        if (cur_texture)
            render_sprites_texture(&sorted_sprites[batch_start], batch_end - batch_start);
        else
            render_sprites_flat(&sorted_sprites[batch_start], batch_end - batch_start);
    };

    for (int i = 1; i < sprite_queue_size_; ++i) {
        const auto p = sorted_sprites[i];

        if (p->blend != cur_blend_mode || p->texture != cur_texture) {
            render_sprites(i);

            batch_start = i;

            if (p->texture != cur_texture || p->program != cur_program) {
                cur_texture = p->texture;
                cur_program = p->program;
                bind_texture(cur_program, cur_texture);
            }

            if (p->blend != cur_blend_mode) {
                cur_blend_mode = p->blend;
                gl_set_blend_mode(cur_blend_mode);
            }
        }
    }

    render_sprites(sprite_queue_size_);

    sprite_queue_size_ = 0;
}

void sprite_batch::render_sprites_texture(const sprite *const *sprites, int num_sprites) const
{
    assert(num_sprites > 0);

    static GLfloat data[SPRITE_QUEUE_CAPACITY*4*8];

    struct {
        GLfloat *dest;

        void operator()(const g2d::vec2& vert, const g2d::vec2& texuv, const g2d::rgba& color)
        {
            *dest++ = vert.x;
            *dest++ = vert.y;

            *dest++ = texuv.x;
            *dest++ = texuv.y;

            *dest++ = color.r;
            *dest++ = color.g;
            *dest++ = color.b;
            *dest++ = color.a;
        }
    } add_vertex { data };

    for (int i = 0; i < num_sprites; ++i) {
        auto p = sprites[i];
        add_vertex(p->verts.v00, p->texcoords.v00, p->color);
        add_vertex(p->verts.v01, p->texcoords.v01, p->color);
        add_vertex(p->verts.v10, p->texcoords.v10, p->color);
        add_vertex(p->verts.v11, p->texcoords.v11, p->color);
    }

    // TODO vao

    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), &data[0]));
    GL_CHECK(glEnableVertexAttribArray(0));

    GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), &data[2]));
    GL_CHECK(glEnableVertexAttribArray(1));

    GL_CHECK(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), &data[4]));
    GL_CHECK(glEnableVertexAttribArray(2));

    // TODO triangles, not quads

    GL_CHECK(glDrawArrays(GL_QUADS, 0, 4*num_sprites));

    GL_CHECK(glDisableVertexAttribArray(2));
    GL_CHECK(glDisableVertexAttribArray(1));
    GL_CHECK(glDisableVertexAttribArray(0));
}

void sprite_batch::render_sprites_flat(const sprite *const *sprites, int num_sprites) const
{
    assert(num_sprites > 0);

    static GLfloat data[SPRITE_QUEUE_CAPACITY*4*6];

    struct {
        GLfloat *dest;

        void operator()(const g2d::vec2& vert, const g2d::rgba& color)
        {
            *dest++ = vert.x;
            *dest++ = vert.y;

            *dest++ = color.r;
            *dest++ = color.g;
            *dest++ = color.b;
            *dest++ = color.a;
        }
    } add_vertex { data };

    for (int i = 0; i < num_sprites; ++i) {
        auto p = sprites[i];
        add_vertex(p->verts.v00, p->color);
        add_vertex(p->verts.v01, p->color);
        add_vertex(p->verts.v10, p->color);
        add_vertex(p->verts.v11, p->color);
    }

    // TODO vao

    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), &data[0]));
    GL_CHECK(glEnableVertexAttribArray(0));

    GL_CHECK(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), &data[2]));
    GL_CHECK(glEnableVertexAttribArray(1));

    // TODO triangles, not quads

    GL_CHECK(glDrawArrays(GL_QUADS, 0, 4*num_sprites));

    GL_CHECK(glDisableVertexAttribArray(1));
    GL_CHECK(glDisableVertexAttribArray(0));
}

}

namespace render {

void init()
{
    g_sprite_batch.init();
}

void set_viewport(int x_min, int x_max, int y_min, int y_max)
{
    g_sprite_batch.set_viewport(x_min, x_max, y_min, y_max);
}

void begin_batch()
{
    g_sprite_batch.begin_batch();
}

void end_batch()
{
    g_sprite_batch.end_batch();
}

void push_matrix()
{
    g_sprite_batch.push_matrix();
}

void pop_matrix()
{
    g_sprite_batch.pop_matrix();
}

void translate(float x, float y)
{
    g_sprite_batch.translate(x, y);
}

void scale(float sx, float sy)
{
    g_sprite_batch.scale(sx, sy);
}

void rotate(float a)
{
    g_sprite_batch.rotate(a);
}

void set_blend_mode(blend_mode mode)
{
    g_sprite_batch.set_blend_mode(mode);
}

void set_color(const g2d::rgba& color)
{
    g_sprite_batch.set_color(color);
}

void draw_quad(const g2d::program *program, const g2d::texture *texture, const quad& verts, const quad& texcoords, int layer)
{
    g_sprite_batch.add_quad(program, texture, verts, texcoords, layer);
}

void draw_quad(const g2d::texture *texture, const quad& verts, const quad& texcoords, int layer)
{
    g_sprite_batch.add_quad(texture, verts, texcoords, layer);
}

void draw_quad(const quad& verts, int layer)
{
    g_sprite_batch.add_quad(verts, layer);
}

void draw_sprite(const g2d::sprite *sprite, float x, float y, int layer)
{
    // TODO eventually move this to g2d::sprite itself

    const auto x0 = x + sprite->left_margin_;
    const auto x1 = x0 + sprite->width_;

    const auto y0 = y + sprite->bottom_margin_;
    const auto y1 = y0 + sprite->height_;

    const auto u0 = sprite->u0_;
    const auto u1 = sprite->u1_;

    const auto v0 = sprite->v0_;
    const auto v1 = sprite->v1_;

    g_sprite_batch.add_quad(
            sprite->texture_,
            { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } },
            { { u0, v1 }, { u1, v1 }, { u1, v0 }, { u0, v0 } },
            layer);
}

}
