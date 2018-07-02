#include <guava2d/rgb.h>
#include <guava2d/texture_manager.h>
#include <guava2d/xwchar.h>

#include "common.h"
#include "render.h"
#include "text_box.h"
#include "fonts.h"

#include <tuple>
#include <algorithm>

namespace {

template <typename Visitor>
void for_each_char(const wchar_t *text, float max_width, Visitor visit)
{
    const g2d::font *fonts[] = { get_font(font::small), get_font(font::tiny) };

    const int font_heights[] = {
        46, 38,
    };

    int cur_font = 0;
    int cur_line_height = font_heights[cur_font];

    float y = 0, x = 0;

    const wchar_t *word_start = nullptr;
    float word_start_x = 0;

    auto add_word = [&] {
        float x = word_start_x;
        const g2d::font *font = fonts[cur_font];

        for (const wchar_t *q = word_start; *q && (*q != ' ' && *q != '\n' && *q != '@'); q++) {
            const g2d::glyph_info *g = font->find_glyph(*q);
            visit(font, g, x, y);
            x += g->advance_x;
        }

        word_start = nullptr;

        return x;
    };

    for (const wchar_t *p = text; *p; p++) {
        if (*p == '@') {
            if (word_start)
                add_word();

            cur_font = static_cast<int>(*++p) - '0';

            if (x == 0)
                cur_line_height = font_heights[cur_font];
            else
                cur_line_height = std::max(font_heights[cur_font], cur_line_height);
        } else if (*p == '\n') {
            if (word_start)
                add_word();

            y -= cur_line_height;
            x = 0;
            cur_line_height = font_heights[cur_font];
        } else {
            if (*p == ' ') {
                if (word_start)
                    add_word();
            } else {
                if (!word_start) {
                    word_start = p;
                    word_start_x = x;
                }
            }

            x += fonts[cur_font]->find_glyph(*p)->advance_x;

            if (x > max_width) {
                word_start_x = 0;

                x = 0;

                if (word_start) {
                    for (const wchar_t *q = word_start; q <= p; q++)
                        x += fonts[cur_font]->find_glyph(*q)->advance_x;
                }

                y -= cur_line_height;
                cur_line_height = font_heights[cur_font];
            }
        }
    }

    if (word_start)
        add_word();
}
};

text_box::text_box(float width)
    : width_(width)
    , height_(0)
    , frame_texture_(g2d::load_texture("images/w-button-border.png"))
{
}

void text_box::set_text(const wchar_t *text)
{
    quads_.clear();

    const float max_width = width_ - 2 * BORDER_RADIUS;

    // figure out bounding box

    bool first = true;

    float x_min = -1, x_max = -1;
    float y_min = -1, y_max = -1;

    for_each_char(text, max_width, [&](const g2d::font *, const g2d::glyph_info *g, float x, float y) {
        const float x_left = x + g->left;
        const float x_right = x_left + g->width;
        const float y_top = y + g->top;
        const float y_bottom = y_top - g->height;

        if (first) {
            x_min = x_left;
            x_max = x_right;

            y_max = y_top;
            y_min = y_bottom;

            first = false;
        } else {
            x_min = std::min(x_left, x_min);
            x_max = std::max(x_right, x_max);

            y_max = std::max(y_top, y_max);
            y_min = std::min(y_bottom, y_min);
        }
    });

    height_ = y_max - y_min + 2 * BORDER_RADIUS;

    const float offs_x = -.5 * width_ + BORDER_RADIUS - x_min;
    const float offs_y = -.5 * height_ + BORDER_RADIUS - y_min;

    // add glyphs

    for_each_char(text, max_width, [&](const g2d::font *font, const g2d::glyph_info *g, float x, float y) {
        const auto &t0 = g->texuv[0];
        const auto &t1 = g->texuv[1];
        const auto &t2 = g->texuv[2];
        const auto &t3 = g->texuv[3];

        const float x_left = x + g->left + offs_x;
        const float x_right = x_left + g->width;
        const float y_top = y + g->top + offs_y;
        const float y_bottom = y_top - g->height;

        const g2d::vec2 p0(x_left, y_top);
        const g2d::vec2 p1(x_right, y_top);
        const g2d::vec2 p2(x_right, y_bottom);
        const g2d::vec2 p3(x_left, y_bottom);

        quads_.emplace_back(g->texture_, render::quad{p0, p1, p2, p3}, render::quad{t0, t1, t2, t3});
    });
}

void text_box::draw(float alpha) const
{
    draw_frame(alpha);
    draw_text(alpha);
}

void text_box::draw_frame(float alpha) const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);
    render::set_color({1.f, 1.f, 1.f, alpha});

    const float x0 = -.5 * width_;
    const float x1 = x0 + BORDER_RADIUS;

    const float x3 = .5 * width_;
    const float x2 = x3 - BORDER_RADIUS;

    const float y0 = .5 * height_;
    const float y1 = y0 - BORDER_RADIUS;

    const float y3 = -.5 * height_;
    const float y2 = y3 + BORDER_RADIUS;

    render::draw_box(frame_texture_, {{x0, y0}, {x1, y1}}, {{0, 0}, {.25, .25}}, 20);
    render::draw_box(frame_texture_, {{x1, y0}, {x2, y1}}, {{.25, 0}, {.75, .25}}, 20);
    render::draw_box(frame_texture_, {{x2, y0}, {x3, y1}}, {{.75, 0}, {1, .25}}, 20);

    render::draw_box(frame_texture_, {{x0, y1}, {x1, y2}}, {{0, .25}, {.25, .75}}, 20);
    render::draw_box(frame_texture_, {{x1, y1}, {x2, y2}}, {{.25, .25}, {.75, .75}}, 20);
    render::draw_box(frame_texture_, {{x2, y1}, {x3, y2}}, {{.75, .25}, {1, .75}}, 20);

    render::draw_box(frame_texture_, {{x0, y2}, {x1, y3}}, {{0, .75}, {.25, 1}}, 20);
    render::draw_box(frame_texture_, {{x1, y2}, {x2, y3}}, {{.25, .75}, {.75, 1}}, 20);
    render::draw_box(frame_texture_, {{x2, y2}, {x3, y3}}, {{.75, .75}, {1, 1}}, 20);
}

void text_box::draw_text(float alpha) const
{
    render::set_blend_mode(blend_mode::INVERSE_BLEND);
    render::set_color({alpha, alpha, alpha, 1.f});

    for (const auto &p : quads_)
        render::draw_quad(std::get<0>(p), std::get<1>(p), std::get<2>(p), 21);
}
