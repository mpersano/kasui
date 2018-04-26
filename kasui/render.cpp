#include "render.h"
#include "noncopyable.h"

#include "gl_check.h"
#include "program_manager.h"

#include <guava2d/font.h>
#include <guava2d/program.h>
#include <guava2d/rgb.h>
#include <guava2d/texture.h>

#include <algorithm>
#include <cassert>
#include <stack>

namespace render {

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

        case blend_mode::INVERSE_BLEND:
            GL_CHECK(glEnable(GL_BLEND));
            GL_CHECK(glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR));
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
    void set_color(const g2d::rgba &color);

    void add_quad(const g2d::program *program, const g2d::texture *texture, const quad &verts, const quad &texcoords,
                  const vert_colors &colors0, const vert_colors &colors1, int layer);
    void add_quad(const g2d::program *program, const g2d::texture *texture, const quad &verts, const quad &texcoords,
                  const vert_colors &colors, int layer);
    void add_quad(const g2d::program *program, const g2d::texture *texture, const quad &verts, const quad &texcoords,
                  int layer);

    void set_text_align(text_align align);
    void add_text(const g2d::font *font, const g2d::vec2 &pos, int layer, const wchar_t *str);
    void add_text(const g2d::font *font, const g2d::vec2 &pos, int layer, const g2d::rgba &outline_color,
                  const g2d::rgba &text_color, const wchar_t *str);

private:
    struct sprite
    {
        int layer;
        const g2d::program *program;
        const g2d::texture *texture;
        int num_vert_colors; // for now always 1 or 2
        quad verts;
        quad texcoords;
        vert_colors colors[2];
        blend_mode blend;
    };

    void load_programs();
    void init_vbo();
    void init_vaos();

    void flush_queue();
    void render_sprites_texture(const sprite *const *sprites, int num_sprites) const;
    void render_sprites_texture_2c(const sprite *const *sprites, int num_sprites) const;
    void render_sprites_flat(const sprite *const *sprites, int num_sprites) const;

    static const int SPRITE_QUEUE_CAPACITY = 1024;

    int sprite_queue_size_;
    sprite sprite_queue_[SPRITE_QUEUE_CAPACITY];

    blend_mode blend_mode_;
    g2d::rgba color_;
    g2d::mat3 matrix_;
    text_align text_align_;
    std::stack<g2d::mat3> matrix_stack_;

    const g2d::program *program_texture_;
    const g2d::program *program_flat_;
    const g2d::program *program_text_outline_;

    GLuint vbo_;
    GLuint vao_flat_;
    GLuint vao_texture_;
    GLuint vao_texture_2c_;

    std::array<GLfloat, 16> proj_matrix_;
} g_sprite_batch;

sprite_batch::sprite_batch()
{
}

void sprite_batch::init()
{
    load_programs();
    init_vbo();
    init_vaos();
}

void sprite_batch::set_viewport(int x_min, int x_max, int y_min, int y_max)
{
    const float a = 2.f / (x_max - x_min);
    const float b = 2.f / (y_max - y_min);

    const float tx = -(x_max + x_min) / (x_max - x_min);
    const float ty = -(y_max + y_min) / (y_max - y_min);

    proj_matrix_ = {a, 0, 0, 0, 0, b, 0, 0, 0, 0, 0, 0, tx, ty, 0, 1};

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
    color_ = {1, 1, 1, 1};
    text_align_ = text_align::LEFT;
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

void sprite_batch::set_color(const g2d::rgba &color)
{
    color_ = color;
}

void sprite_batch::add_quad(const g2d::program *program, const g2d::texture *texture, const quad &verts,
                            const quad &texcoords, const vert_colors &colors0, const vert_colors &colors1, int layer)
{
    if (sprite_queue_size_ == SPRITE_QUEUE_CAPACITY)
        flush_queue();

    auto &s = sprite_queue_[sprite_queue_size_++];

    s.program = program;
    s.texture = texture;

    s.verts.v00 = matrix_ * verts.v00;
    s.verts.v01 = matrix_ * verts.v01;
    s.verts.v10 = matrix_ * verts.v10;
    s.verts.v11 = matrix_ * verts.v11;

    s.texcoords = texcoords;

    s.layer = layer;
    s.blend = blend_mode_;

    s.num_vert_colors = 2;
    s.colors[0] = colors0;
    s.colors[1] = colors1;
}

void sprite_batch::add_quad(const g2d::program *program, const g2d::texture *texture, const quad &verts,
                            const quad &texcoords, const vert_colors &colors, int layer)
{
    if (sprite_queue_size_ == SPRITE_QUEUE_CAPACITY)
        flush_queue();

    auto &s = sprite_queue_[sprite_queue_size_++];

    s.program = program;
    s.texture = texture;

    s.verts.v00 = matrix_ * verts.v00;
    s.verts.v01 = matrix_ * verts.v01;
    s.verts.v10 = matrix_ * verts.v10;
    s.verts.v11 = matrix_ * verts.v11;

    s.texcoords = texcoords;

    s.layer = layer;
    s.blend = blend_mode_;

    s.num_vert_colors = 1;
    s.colors[0] = colors;
}

void sprite_batch::add_quad(const g2d::program *program, const g2d::texture *texture, const quad &verts,
                            const quad &texcoords, int layer)
{
    add_quad(program, texture, verts, texcoords, {color_, color_, color_, color_}, layer);
}

void sprite_batch::set_text_align(text_align align)
{
    text_align_ = align;
}

void sprite_batch::add_text(const g2d::font *font, const g2d::vec2 &pos, int layer, const wchar_t *str)
{
    float x = pos.x;

    if (text_align_ == text_align::RIGHT)
        x -= font->get_string_width(str);
    else if (text_align_ == text_align::CENTER)
        x -= .5f * font->get_string_width(str);

    const auto texture = font->get_texture();

    for (const wchar_t *p = str; *p; ++p) {
        const auto g = font->find_glyph(*p);

        const float x0 = x + g->left;
        const float x1 = x0 + g->width;
        const float y0 = pos.y + g->top;
        const float y1 = y0 - g->height;

        add_quad(nullptr, texture, {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}},
                 {g->texuv[0], g->texuv[1], g->texuv[2], g->texuv[3]}, layer);

        x += g->advance_x;
    }
}

void sprite_batch::add_text(const g2d::font *font, const g2d::vec2 &pos, int layer, const g2d::rgba &outline_color,
                            const g2d::rgba &text_color, const wchar_t *str)
{
    float x = pos.x;

    if (text_align_ == text_align::RIGHT)
        x -= font->get_string_width(str);
    else if (text_align_ == text_align::CENTER)
        x -= .5f * font->get_string_width(str);

    const auto texture = font->get_texture();

    for (const wchar_t *p = str; *p; ++p) {
        const auto g = font->find_glyph(*p);

        const float x0 = x + g->left;
        const float x1 = x0 + g->width;
        const float y0 = pos.y + g->top;
        const float y1 = y0 - g->height;

        add_quad(program_text_outline_, texture, {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}},
                 {g->texuv[0], g->texuv[1], g->texuv[2], g->texuv[3]},
                 {outline_color, outline_color, outline_color, outline_color},
                 {text_color, text_color, text_color, text_color}, layer);

        x += g->advance_x;
    }
}

void sprite_batch::load_programs()
{
    program_texture_ = load_program("shaders/sprite.vert", "shaders/sprite.frag");
    program_flat_ = load_program("shaders/flat.vert", "shaders/flat.frag");
    program_text_outline_ = load_program("shaders/sprite_2c.vert", "shaders/text_outline.frag");
}

void sprite_batch::init_vbo()
{
    GL_CHECK(glGenBuffers(1, &vbo_));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, SPRITE_QUEUE_CAPACITY * 4 * 8 * sizeof(GLfloat), nullptr, GL_DYNAMIC_DRAW));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void sprite_batch::init_vaos()
{
    auto enable_vertex_attrib_array = [](GLuint index, GLint size, GLsizei stride, int offset) {
        GL_CHECK(glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride,
                                       reinterpret_cast<GLvoid *>(offset * sizeof(GLfloat))));
        GL_CHECK(glEnableVertexAttribArray(index));
    };

    GL_CHECK(glGenVertexArrays(1, &vao_flat_));
    GL_CHECK(glBindVertexArray(vao_flat_));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    enable_vertex_attrib_array(0, 2, 6 * sizeof(GLfloat), 0);
    enable_vertex_attrib_array(1, 4, 6 * sizeof(GLfloat), 2);

    GL_CHECK(glGenVertexArrays(1, &vao_texture_));
    GL_CHECK(glBindVertexArray(vao_texture_));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    enable_vertex_attrib_array(0, 2, 8 * sizeof(GLfloat), 0);
    enable_vertex_attrib_array(1, 2, 8 * sizeof(GLfloat), 2);
    enable_vertex_attrib_array(2, 4, 8 * sizeof(GLfloat), 4);

    GL_CHECK(glGenVertexArrays(1, &vao_texture_2c_));
    GL_CHECK(glBindVertexArray(vao_texture_2c_));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    enable_vertex_attrib_array(0, 2, 12 * sizeof(GLfloat), 0);
    enable_vertex_attrib_array(1, 2, 12 * sizeof(GLfloat), 2);
    enable_vertex_attrib_array(2, 4, 12 * sizeof(GLfloat), 4);
    enable_vertex_attrib_array(3, 4, 12 * sizeof(GLfloat), 8);
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
    auto cur_num_vert_colors = sorted_sprites[0]->num_vert_colors;

    bind_texture(cur_program, cur_texture);
    gl_set_blend_mode(cur_blend_mode);

    int batch_start = 0;

    const auto render_sprites = [&](int batch_end) {
        if (cur_texture) {
            if (cur_num_vert_colors == 1)
                render_sprites_texture(&sorted_sprites[batch_start], batch_end - batch_start);
            else
                render_sprites_texture_2c(&sorted_sprites[batch_start], batch_end - batch_start);
        } else {
            render_sprites_flat(&sorted_sprites[batch_start], batch_end - batch_start);
        }
    };

    for (int i = 1; i < sprite_queue_size_; ++i) {
        const auto p = sorted_sprites[i];

        if (p->blend != cur_blend_mode || p->texture != cur_texture || p->program != cur_program ||
            p->num_vert_colors != cur_num_vert_colors) {
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

            cur_num_vert_colors = p->num_vert_colors;
        }
    }

    render_sprites(sprite_queue_size_);

    sprite_queue_size_ = 0;
}

void sprite_batch::render_sprites_texture(const sprite *const *sprites, int num_sprites) const
{
    assert(num_sprites > 0);

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));

    struct
    {
        GLfloat *dest = reinterpret_cast<GLfloat *>(GL_CHECK_R(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));

        void operator()(const g2d::vec2 &vert, const g2d::vec2 &texuv, const g2d::rgba &color)
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
    } add_vertex;

    for (int i = 0; i < num_sprites; ++i) {
        auto p = sprites[i];
        add_vertex(p->verts.v00, p->texcoords.v00, p->colors[0].c00);
        add_vertex(p->verts.v01, p->texcoords.v01, p->colors[0].c01);
        add_vertex(p->verts.v10, p->texcoords.v10, p->colors[0].c10);
        add_vertex(p->verts.v11, p->texcoords.v11, p->colors[0].c11);
    }

    GL_CHECK(glUnmapBuffer(GL_ARRAY_BUFFER));

    GL_CHECK(glBindVertexArray(vao_texture_));
    GL_CHECK(glDrawArrays(GL_QUADS, 0, 4 * num_sprites)); // TODO triangles, not quads
}

void sprite_batch::render_sprites_texture_2c(const sprite *const *sprites, int num_sprites) const
{
    assert(num_sprites > 0);

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));

    struct
    {
        GLfloat *dest = reinterpret_cast<GLfloat *>(GL_CHECK_R(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));

        void operator()(const g2d::vec2 &vert, const g2d::vec2 &texuv, const g2d::rgba &color0, const g2d::rgba &color1)
        {
            *dest++ = vert.x;
            *dest++ = vert.y;

            *dest++ = texuv.x;
            *dest++ = texuv.y;

            *dest++ = color0.r;
            *dest++ = color0.g;
            *dest++ = color0.b;
            *dest++ = color0.a;

            *dest++ = color1.r;
            *dest++ = color1.g;
            *dest++ = color1.b;
            *dest++ = color1.a;
        }
    } add_vertex;

    for (int i = 0; i < num_sprites; ++i) {
        auto p = sprites[i];
        add_vertex(p->verts.v00, p->texcoords.v00, p->colors[0].c00, p->colors[1].c00);
        add_vertex(p->verts.v01, p->texcoords.v01, p->colors[0].c01, p->colors[1].c01);
        add_vertex(p->verts.v10, p->texcoords.v10, p->colors[0].c10, p->colors[1].c10);
        add_vertex(p->verts.v11, p->texcoords.v11, p->colors[0].c11, p->colors[1].c11);
    }

    GL_CHECK(glUnmapBuffer(GL_ARRAY_BUFFER));

    GL_CHECK(glBindVertexArray(vao_texture_2c_));
    GL_CHECK(glDrawArrays(GL_QUADS, 0, 4 * num_sprites)); // TODO triangles, not quads
}

void sprite_batch::render_sprites_flat(const sprite *const *sprites, int num_sprites) const
{
    assert(num_sprites > 0);

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));

    struct
    {
        GLfloat *dest = reinterpret_cast<GLfloat *>(GL_CHECK_R(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));

        void operator()(const g2d::vec2 &vert, const g2d::rgba &color)
        {
            *dest++ = vert.x;
            *dest++ = vert.y;

            *dest++ = color.r;
            *dest++ = color.g;
            *dest++ = color.b;
            *dest++ = color.a;
        }
    } add_vertex;

    for (int i = 0; i < num_sprites; ++i) {
        auto p = sprites[i];
        add_vertex(p->verts.v00, p->colors[0].c00);
        add_vertex(p->verts.v01, p->colors[0].c01);
        add_vertex(p->verts.v10, p->colors[0].c10);
        add_vertex(p->verts.v11, p->colors[0].c11);
    }

    GL_CHECK(glUnmapBuffer(GL_ARRAY_BUFFER));

    GL_CHECK(glBindVertexArray(vao_flat_));
    GL_CHECK(glDrawArrays(GL_QUADS, 0, 4 * num_sprites)); // TODO triangles, not quads
}

} // anonymous namespace

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

void translate(const g2d::vec2 &v)
{
    g_sprite_batch.translate(v.x, v.y);
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

void set_color(const g2d::rgba &color)
{
    g_sprite_batch.set_color(color);
}

void draw_quad(const g2d::program *program, const g2d::texture *texture, const quad &verts, const quad &texcoords,
               int layer)
{
    g_sprite_batch.add_quad(program, texture, verts, texcoords, layer);
}

void draw_quad(const g2d::texture *texture, const quad &verts, const quad &texcoords, int layer)
{
    g_sprite_batch.add_quad(nullptr, texture, verts, texcoords, layer);
}

void draw_box(const g2d::texture *texture, const box &verts, const box &texcoords, int layer)
{
    draw_box(nullptr, texture, verts, texcoords, layer);
}

void draw_box(const g2d::program *program, const g2d::texture *texture, const box &verts, const box &texcoords, int layer)
{
    const float x0 = verts.v0.x;
    const float y0 = verts.v0.y;

    const float x1 = verts.v1.x;
    const float y1 = verts.v1.y;

    const float u0 = texcoords.v0.x;
    const float v0 = texcoords.v0.y;

    const float u1 = texcoords.v1.x;
    const float v1 = texcoords.v1.y;

    g_sprite_batch.add_quad(program, texture, {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}},
                            {{u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}}, layer);
}

void draw_quad(const quad &verts, int layer)
{
    g_sprite_batch.add_quad(nullptr, nullptr, verts, {}, layer);
}

void draw_quad(const g2d::program *program, const g2d::texture *texture, const quad &verts, const quad &texcoords,
               const vert_colors &colors0, const vert_colors &colors1, int layer)
{
    g_sprite_batch.add_quad(program, texture, verts, texcoords, colors0, colors1, layer);
}

void draw_quad(const g2d::program *program, const g2d::texture *texture, const quad &verts, const quad &texcoords,
               const vert_colors &colors, int layer)
{
    g_sprite_batch.add_quad(program, texture, verts, texcoords, colors, layer);
}

void draw_quad(const g2d::texture *texture, const quad &verts, const quad &texcoords, const vert_colors &colors,
               int layer)
{
    g_sprite_batch.add_quad(nullptr, texture, verts, texcoords, colors, layer);
}

void draw_quad(const quad &verts, const vert_colors &colors, int layer)
{
    g_sprite_batch.add_quad(nullptr, nullptr, verts, {}, colors, layer);
}

void set_text_align(text_align align)
{
    g_sprite_batch.set_text_align(align);
}

void draw_text(const g2d::font *font, const g2d::vec2 &pos, int layer, const wchar_t *str)
{
    g_sprite_batch.add_text(font, pos, layer, str);
}

void draw_text(const g2d::font *font, const g2d::vec2 &pos, int layer, const g2d::rgba &outline_color,
               const g2d::rgba &text_color, const wchar_t *str)
{
    g_sprite_batch.add_text(font, pos, layer, outline_color, text_color, str);
}
}
