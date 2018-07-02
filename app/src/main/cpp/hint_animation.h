#pragma once

#include <guava2d/vec2.h>

#include "common.h"
#include "text_box.h"
#include "world.h"

class jukugo;
class gradient;

namespace g2d {
class mat4;
class texture;
};

class hint_text_box : public sprite
{
public:
    hint_text_box(const hint &h, float cell_size, float width, const gradient& g);
    ~hint_text_box();

    float get_height() const { return text_box_.get_height(); }

    float get_width() const { return width_; }

    float get_alpha() const;

    void set_pos(const g2d::vec2 &p) { pos_ = p; }

    bool update(uint32_t dt) override;
    void draw() const override;

    bool close();

private:
    float get_scale() const;

    g2d::vec2 pos_;
    float width_;
    text_box text_box_;

    int state_tics_, tics_;
    enum class state
    {
        INTRO,
        ACTIVE,
        OUTRO
    } state_;
    gradient gradient_;

    g2d::vec2 from_pos_, to_pos_;

    const g2d::texture *block_texture_;
    render::box block_quad_;
    render::box block_texuv_;

    const g2d::texture *arrow_texture_;
    render::quad arrow_quad_;

    static constexpr int INTRO_TICS = 30 * MS_PER_TIC;
    static constexpr int OUTRO_TICS = 20 * MS_PER_TIC;
};
