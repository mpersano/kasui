#include "clouds_theme.h"
#include "falling_leaves_theme.h"
#include "flowers_theme.h"

theme_animation::~theme_animation()
{
}

std::unique_ptr<theme_animation> make_theme(int index)
{
    return std::unique_ptr<theme_animation>([index]() -> theme_animation * {
        switch (index)
        {
        case THEME_CLOUDS:
            return new clouds_theme;
        case THEME_FALLING_LEAVES:
            return new falling_leaves_theme;
        case THEME_FLOWERS:
        default:
            return new flowers_theme;
        }
    }());
}
