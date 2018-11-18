#pragma once

#include <guava2d/font.h>

enum class font
{
    micro,
    tiny,
    small,
    medium,
    large,
    gameover,
    title,
    font_count
};

const g2d::font *get_font(font f);
