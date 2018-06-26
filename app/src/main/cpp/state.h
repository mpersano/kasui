#pragma once

#include <cstdint>

#include "kasui.h"

class state
{
public:
    virtual ~state() = default;

    virtual void reset() = 0;

    virtual void redraw() const = 0;
    virtual void update(uint32_t dt) = 0;
    virtual void on_touch_down(float x, float y) = 0;
    virtual void on_touch_up() = 0;
    virtual void on_touch_move(float x, float y) = 0;
    virtual void on_back_key() = 0;
    virtual void on_menu_key() = 0;
};
