#pragma once

#include "common.h"
#include "settings.h"
#include "world.h"

#include <string>

namespace g2d
{
class font;
};

class combo_sprite : public sprite
{
public:
    combo_sprite(int combo_size, float x, float y, const gradient& g);

    bool update(uint32_t dt) override;
    void draw() const override;

private:
    static constexpr int TTL = 60 * MS_PER_TIC;
    static constexpr int DIGIT_WIDTH = 25;

    float x_origin_, y_origin_;
    float y_offset_ = 0;
    float x_chain_text_;
    gradient gradient_;
    const g2d::font *large_font_;
    const g2d::font *small_font_;
    int ttl_ = TTL;
    std::wstring combo_size_;
};
