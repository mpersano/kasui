#pragma once

#include <functional>

namespace g2d {
class sprite;
class mat4;
}

class pause_button
{
public:
    pause_button(float x, float y);

    void reset();
    void draw(float alpha) const;

    bool on_touch_down(float x, float y);
    bool on_touch_up();

    void set_callback(std::function<void(void)> callback);

    static const int SIZE = 80;

private:
    float x_base, y_base;

    std::function<void(void)> callback_;

    const g2d::sprite *sprite_unselected_;
    const g2d::sprite *sprite_selected_;

    bool is_selected;
};
