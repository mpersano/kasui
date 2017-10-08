#pragma once

#include <array>

#include <guava2d/g2dgl.h>
#include <guava2d/vec2.h>
#include <guava2d/rgb.h>

namespace g2d {
class program;
class texture;
class sprite;
}

enum class blend_mode { NO_BLEND, ALPHA_BLEND, ADDITIVE_BLEND };

namespace render {

struct quad
{
    g2d::vec2 v00, v01, v10, v11;
};

struct vert_colors
{
    g2d::rgba c00, c01, c10, c11;
};

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

void draw_quad(const quad& verts, int layer);
void draw_quad(const quad& verts, const vert_colors& colors, int layer);

void draw_quad(const g2d::texture *texture, const quad& verts, const quad& texcoords, int layer);
void draw_quad(const g2d::texture *texture, const quad& verts, const quad& texcoords, const vert_colors& colors, int layer);

void draw_quad(const g2d::program *program, const g2d::texture *texture, const quad& verts, const quad& texcoords, int layer);
void draw_quad(const g2d::program *program, const g2d::texture *texture, const quad& verts, const quad& texcoords, const vert_colors& colors, int layer);

}
