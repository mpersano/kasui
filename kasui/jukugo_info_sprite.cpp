#include "jukugo_info_sprite.h"

#include <guava2d/font_manager.h>

#include "common.h"
#include "settings.h"
#include "jukugo.h"
#include "line_splitter.h"
#include "program_manager.h"
#include "render.h"
#include "tween.h"

jukugo_info_sprite::jukugo_info_sprite(const jukugo *jukugo_info, float x, float y, const gradient *g)
    : jukugo_{jukugo_info}
    , x_base_{x}
    , y_base_{y}
    , gradient_{g}
    , program_{load_program("shaders/sprite_2c.vert", "shaders/text_outline.frag")}
{
    action_.reset(
        (new parallel_action_group)
            ->add((new sequential_action_group)
                      ->add(new property_change_action<out_bounce_tween<float>>(&flip_, 0, .5 * M_PI, 30 * MS_PER_TIC))
                      ->add(new delay_action(30 * MS_PER_TIC)))
            ->add((new sequential_action_group)
                      ->add(new delay_action(60 * MS_PER_TIC))
                      ->add(new property_change_action<in_back_tween<float>>(&z_, 0, 1, 20 * MS_PER_TIC)))
            ->add((new sequential_action_group)
                      ->add(new property_change_action<linear_tween<float>>(&alpha_, 0, 1., 10 * MS_PER_TIC))
                      ->add(new delay_action(60 * MS_PER_TIC))
                      ->add(new property_change_action<linear_tween<float>>(&alpha_, 1., 0, 10 * MS_PER_TIC))));

    kanji_font_ = g2d::font_manager::get_instance().load("fonts/large");
    eigo_font_ = g2d::font_manager::get_instance().load("fonts/tiny");
    furigana_font_ = g2d::font_manager::get_instance().load("fonts/small");

    pos_kanji_ = {-.5f * kanji_font_->get_string_width(jukugo_info->kanji), -20.f};
    pos_furigana_ = {-.5f * furigana_font_->get_string_width(jukugo_info->reading), 64.f};

    constexpr int MAX_LINE_WIDTH = 320;
    float y_eigo = -64;

    line_splitter ls(eigo_font_, jukugo_info->eigo);

    std::wstring line;
    while (line = ls.next_line(MAX_LINE_WIDTH), !line.empty()) {
        const float x = -.5 * eigo_font_->get_string_width(line.c_str());
        eigo_lines_.emplace_back(g2d::vec2(x, y_eigo), std::move(line));
        y_eigo -= 44;
    }
}

bool jukugo_info_sprite::update(uint32_t dt)
{
    action_->step(dt);
    return !action_->done();
}

void jukugo_info_sprite::draw() const
{
    const auto top_color = gradient_->to * (1./255.);
    const auto bottom_color = gradient_->from * (1./255.);

    const g2d::rgba top_color_text{top_color, alpha_};
    const g2d::rgba bottom_color_text{bottom_color, alpha_};

    const g2d::rgba top_color_outline{.5*top_color, alpha_};
    const g2d::rgba bottom_color_outline{.5*bottom_color, alpha_};

    const float scale = 1. / (1. + z_);
    const float sy = sinf(flip_);

    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    render::push_matrix();

    render::translate(x_base_, y_base_);
    render::scale(scale, scale * sy);

    render::set_text_align(text_align::LEFT);

    render::draw_text(furigana_font_, pos_furigana_, 50, top_color_outline, top_color_text, jukugo_->reading);
    render::draw_text(kanji_font_, pos_kanji_, 50, top_color_outline, top_color_text, bottom_color_outline,
            bottom_color_text, jukugo_->kanji);

    for (const auto &p : eigo_lines_)
        render::draw_text(eigo_font_, p.first, 50, bottom_color_outline, bottom_color_text, p.second.c_str());

    render::pop_matrix();
}
