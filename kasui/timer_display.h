#pragma once

class timer_display
{
public:
    timer_display();

    void reset(int tics);
    void draw(float alpha) const;
    void update(uint32_t dt);

    int get_tics_left() const { return tics_left; }

    bool finished() const { return tics_left == 0; }

    bool blinking() const { return tics_left <= MIN_SAFE_SECS * 1000; }

private:
    void draw_glyph(const g2d::glyph_info *g, float x, float y, float scale) const;

    enum
    {
        MIN_SAFE_SECS = 10
    };
    int tics_left;

    const g2d::glyph_info *digit_glyphs_[10];
    const g2d::glyph_info *byou_;
    const g2d::texture *texture_;
};
