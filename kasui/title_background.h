#pragma once

#include <memory>

#include "sakura.h"

class title_background
{
public:
    title_background();
    ~title_background();

    void reset();
    void update(uint32_t dt);
    void draw() const;

    void show_billboard();
    void hide_billboard();

    void show_logo();
    void hide_logo();

    class widget;

private:
    void draw_background_quad() const;

    std::unique_ptr<widget> billboard_;
    std::unique_ptr<widget> logo_;

    const g2d::texture *bg_texture_;

    sakura_fubuki sakura_;
};
