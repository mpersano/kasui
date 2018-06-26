#pragma once

#include <cstdint>
#include <memory>

class theme_animation
{
public:
    virtual ~theme_animation();

    virtual void reset() = 0;
    virtual void draw() const = 0;
    virtual void update(uint32_t dt) = 0;
};

enum
{
    THEME_CLOUDS,
    THEME_FALLING_LEAVES,
    THEME_FLOWERS,
    NUM_THEMES,
};

std::unique_ptr<theme_animation> make_theme(int index);
