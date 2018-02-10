#include "clouds_theme.h"
#include "falling_leaves_theme.h"
#include "flowers_theme.h"

theme *themes[NUM_THEMES] = {
    &clouds_theme, &falling_leaves_theme, &flowers_theme,
};

void themes_initialize()
{
    for (auto &theme : themes)
        theme->initialize();
}
