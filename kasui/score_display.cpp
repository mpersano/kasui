#include "score_display.h"

#include "common.h"
#include "in_game.h"
#include "render.h"
#include "program_manager.h"

#include <guava2d/font_manager.h>
#include <guava2d/rgb.h>

#include <cassert>

enum
{
    DIGIT_WIDTH = 36,
    START_DIGIT_BUMP = 10 * MS_PER_TIC,
    VALUE_DEQUEUE_TICS = 5 * MS_PER_TIC,
};

score_display::score_display()
    : program_{load_program("shaders/sprite_2c.vert", "shaders/text_outline.frag")}
{
    reset();

    const g2d::font *font = g2d::font_manager::get_instance().load("fonts/medium");
    for (int i = 0; i < 10; i++)
        digit_glyphs_[i] = font->find_glyph(L'0' + i);

    texture_ = font->get_texture();
}

void score_display::reset()
{
    for (digit *p = digits; p != &digits[NUM_DIGITS]; p++) {
        p->value = 0;
        p->bump = 0;
    }

    queue_head = queue_tail = queue_size = 0;
}

void score_display::set_value(int value)
{
    for (digit *p = digits; p != &digits[NUM_DIGITS] && value; p++, value /= 10) {
        int d = value % 10;

        if (d != p->value) {
            p->value = d;
            p->bump = START_DIGIT_BUMP;
        }
    }
}

void score_display::enqueue_value(int value)
{
    assert(queue_size < MAX_QUEUE_SIZE);

    queue[queue_tail].tics = VALUE_DEQUEUE_TICS;
    queue[queue_tail].value = value;

    if (++queue_tail == MAX_QUEUE_SIZE)
        queue_tail = 0;
    ++queue_size;
}

void score_display::update(uint32_t dt)
{
    for (digit *p = digits; p != &digits[NUM_DIGITS]; p++) {
        if (p->bump)
            p->bump -= dt;
    }

    if (queue_size > 0) {
        if ((queue[queue_head].tics -= dt) <= 0) {
            set_value(queue[queue_head].value);
            if (++queue_head == MAX_QUEUE_SIZE)
                queue_head = 0;
            --queue_size;
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

    while (num_digits > 1 && digits[num_digits - 1].value == 0)
        --num_digits;

    for (const digit *p = digits; p != &digits[num_digits]; p++) {
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
