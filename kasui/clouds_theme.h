#pragma once

#include "theme.h"

class clouds_theme : public theme
{
public:
    clouds_theme();

    void reset() override;
    void draw() const override;
    void update(uint32_t dt) override;
};
