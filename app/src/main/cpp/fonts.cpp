#include "fonts.h"

#include <vector>

const g2d::font *get_font(font f)
{
    static const auto fonts = [] {
        static const char *sources[] = {
            "fonts/micro",
            "fonts/tiny",
            "fonts/small",
            "fonts/medium",
            "fonts/large",
            "fonts/gameover",
            "fonts/title"
        };
        std::vector<g2d::font *> fonts;
        fonts.reserve(static_cast<size_t>(font::font_count));
        for (auto source : sources)
            fonts.push_back(new g2d::font{source});
        return fonts;
    }();
    return fonts[static_cast<int>(f)];
}
