#include "clouds_theme.h"
#include "falling_leaves_theme.h"
#include "flowers_theme.h"

theme::~theme()
{
}

std::unique_ptr<theme> make_theme(int index)
{
    return std::unique_ptr<theme>([index]() -> theme * {
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
