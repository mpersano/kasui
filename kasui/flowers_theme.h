#pragma once

#include "theme.h"

class flowers_theme : public theme_animation
{
public:
    flowers_theme();

    void reset() override;
    void draw() const override;
    void update(uint32_t dt) override;
};
