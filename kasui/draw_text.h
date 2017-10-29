#pragma once

#include <cstddef>

namespace g2d {
class font;
class vec2;
}

void draw_text(const g2d::font *font, const g2d::vec2& pos, const wchar_t *str);
