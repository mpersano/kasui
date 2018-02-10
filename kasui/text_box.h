#pragma once

#include "render.h"

namespace g2d {
class texture;
};

class text_box
{
public:
    text_box(float width);

    void set_text(const wchar_t *text);

    void draw(float alpha) const;

    float get_height() const { return height_; }

    static const int HEIGHT = 200;

private:
    static const int BORDER_RADIUS = 16;
    static const int LINE_HEIGHT = 46;

    void draw_frame(float alpha) const;
    void draw_text(float alpha) const;

    float width_, height_;
    const g2d::texture *frame_texture_;
    std::vector<std::tuple<const g2d::texture *, render::quad, render::quad>> quads_;
};
