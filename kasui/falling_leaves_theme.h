#pragma once

#include "theme.h"

class falling_leaves_theme : public theme_animation
{
public:
    falling_leaves_theme();

    void reset() override;
    void draw() const override;
    void update(uint32_t dt) override;
};
