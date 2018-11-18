#include "score_display.h"

#include "common.h"
#include "in_game.h"
#include "render.h"
#include "programs.h"
#include "fonts.h"

#include <guava2d/rgb.h>

#include <cassert>

enum
{
    DIGIT_WIDTH = 36,
    START_DIGIT_BUMP = 10 * MS_PER_TIC,
    VALUE_DEQUEUE_TICS = 5 * MS_PER_TIC,
};

score_display::score_display()
    : program_{get_program(program::text_gradient)}
{
    reset();

    const auto *f = get_font(font::medium);
    for (int i = 0; i < 10; i++)
        digit_glyphs_[i] = f->find_glyph(L'0' + i);

    texture_ = f->get_texture();
}

void score_display::reset()
{
    for (auto& digit : digits_) {
        digit.value = 0;
        digit.bump = 0;
    }
}

void score_display::set_value(int value)
{
    for (auto& digit : digits_) {
        int d = value % 10;

        if (d != digit.value) {
            digit.value = d;
            digit.bump = START_DIGIT_BUMP;
        }

        value /= 10;
        if (value == 0)
            break;
    }
}

void score_display::enqueue_value(int value)
{
    queue_.push_back({VALUE_DEQUEUE_TICS, value});
}

void score_display::update(uint32_t dt)
{
    for (auto& digit : digits_) {
        if (digit.bump)
            digit.bump -= dt;
    }

    if (!queue_.empty()) {
        auto& front = queue_.front();
        front.tics -= dt;
        if (front.tics <= 0) {
            set_value(front.value);
            queue_.pop_front();
        }
    }
}

void score_display::draw(float x_center, float y_center) const
{
    constexpr float SCORE_SCALE = .8;

    render::set_blend_mode(blend_mode::ALPHA_BLEND);

	const g2d::rgba outline_color{1, 1, 1, 1};
	const g2d::rgba text_color{0, 0, 0, 1};

    int num_digits = NUM_DIGITS;

    while (num_digits > 1 && digits_[num_digits - 1].value == 0)
        --num_digits;

    for (const auto *p = &digits_[0]; p != &digits_[num_digits]; p++) {
        const float fbump = p->bump > 0 ? static_cast<float>(p->bump) / MS_PER_TIC : 0;

        const float scale = SCORE_SCALE / (1. - .05 * fbump);

        const auto g = digit_glyphs_[p->value];

        const float dx = .5 * g->width * scale;
        const float dy = .5 * g->height * scale;

        const float x0 = x_center - dx;
        const float x1 = x_center + dx;

        const float y0 = y_center + dy;
        const float y1 = y_center - dy;

        render::draw_quad(program_, texture_, {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}},
                          {g->texuv[0], g->texuv[1], g->texuv[2], g->texuv[3]},
                          {outline_color, outline_color, outline_color, outline_color},
                          {text_color, text_color, text_color, text_color},
                          50);

        x_center -= DIGIT_WIDTH * SCORE_SCALE;
    }
}
