#pragma once

#include <array>
#include <deque>
#include <cstdint>

namespace g2d {
class glyph_info;
class texture;
class program;
};

class score_display
{
public:
    score_display();

    void reset();
    void enqueue_value(int value);
    void set_value(int value);

    void draw(float x_base, float y_base) const;
    void update(uint32_t dt);

private:
    static constexpr int NUM_DIGITS = 6;
    struct digit
    {
        int value;
        int bump;
    };
    std::array<digit, NUM_DIGITS> digits_;

    struct value_change
    {
        int tics;
        int value;
    };
    std::deque<value_change> queue_;

    std::array<const g2d::glyph_info *, 10> digit_glyphs_;
    const g2d::texture *texture_;
    const g2d::program *program_;
};
