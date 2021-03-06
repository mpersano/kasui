#include "in_game.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <sstream>
#include <vector>

#include <stdlib.h>
#include <wchar.h>

#include "guava2d/program.h"
#include "guava2d/rgb.h"
#include "guava2d/texture_manager.h"
#include "guava2d/vec2.h"
#include "guava2d/xwchar.h"

#include "action.h"
#include "background.h"
#include "common.h"
#include "hiscore_input.h"
#include "jukugo.h"
#include "line_splitter.h"
#include "main_menu.h"
#include "options.h"
#include "pause_button.h"
#include "programs.h"
#include "render.h"
#include "score_display.h"
#include "sounds.h"
#include "sprite_manager.h"
#include "theme.h"
#include "timer_display.h"
#include "tween.h"
#include "world.h"
#include "settings.h"
#include "fonts.h"

float grid_base_x, grid_base_y;

int total_tics;

enum
{
    DEAD_BLOCK_TTL = 30,

    FADE_OUT_TICS = 80 * MS_PER_TIC,
    FADE_IN_TICS = 10 * MS_PER_TIC,

    LEVEL_COMPLETED_TICS = 120 * MS_PER_TIC,

    FLARE_TEXTURE_ROWS = 4,
    FLARE_NUM_FRAMES = FLARE_TEXTURE_ROWS * FLARE_TEXTURE_ROWS,
    FLARE_FRAME_SIZE = 256,
    FLARE_TICS = FLARE_NUM_FRAMES,
};

struct game_animation
{
    virtual void draw() const = 0;
    virtual bool update(uint32_t dt);

    std::unique_ptr<abstract_action> action;
};

struct glyph_animation : game_animation
{
    glyph_animation(const g2d::font *font, float glyph_spacing, const wchar_t *message, float x_base, float y_base,
                    const gradient& g);

    void draw() const override;

    struct glyph_state
    {
        glyph_state(const g2d::glyph_info *gi, bool flip, float x_center, float y_center, const gradient& g);

        void draw(const g2d::program *program, float a) const;

        g2d::vec2 p0, p1, p2, p3;
        g2d::vec2 t0, t1, t2, t3;
        const g2d::texture *texture;
        float x_center, y_center;
        float flip; // in radians
        float z;
        float glyph_alpha;
        g2d::rgb top_color_text, top_color_outline;
        g2d::rgb bottom_color_text, bottom_color_outline;
    };

    std::vector<glyph_state> glyph_states;

    float alpha;
    float scale;
    float x_base;
    float y_base;
    const gradient gradient_;
    const g2d::program *program_;
};

struct level_intro_animation : glyph_animation
{
    level_intro_animation(const gradient& g);
};

struct level_completed_animation : glyph_animation
{
    level_completed_animation(const gradient& g);
};

struct abunai_animation : game_animation
{
    abunai_animation();

    void draw() const override;

    float x, alpha;
    const g2d::sprite *abunai_bb_;
};

struct game_over_animation : glyph_animation
{
    game_over_animation(const g2d::font *font, float spacing, const wchar_t *str, const gradient& g);

    void draw() const override;

    float billboard_x, overlay_alpha;
    const g2d::sprite *game_over_bb_;
};

struct countdown_digit : game_animation
{
    countdown_digit(const g2d::glyph_info *gi, float x_center, float y_center);

    void draw() const override;

    const g2d::texture *texture;

    float x_center, y_center;
    float z;
    float alpha;
    render::quad verts;
    render::quad texcoords;
};

bool game_animation::update(uint32_t dt)
{
    action->step(dt);
    return !action->done();
}

glyph_animation::glyph_animation(const g2d::font *font, float glyph_spacing, const wchar_t *message, float x_base,
                                 float y_base, const gradient& g)
    : alpha(1)
    , scale(1)
    , x_base(x_base)
    , y_base(y_base)
    , gradient_(g)
    , program_(get_program(program::text_gradient))
{
    int num_glyphs = xwcslen(message);

    float char_height = 0;

    for (int i = 0; i < num_glyphs; i++) {
        int h = font->find_glyph(message[i])->height;
        if (h > char_height)
            char_height = h;
    }

    char_height *= glyph_spacing;

    float y = .5 * (num_glyphs - 1) * char_height;

    for (int i = 0; i < num_glyphs; i++) {
        const wchar_t ch = message[i];
        glyph_states.emplace_back(font->find_glyph(ch), ch == L'〜', 0, y, g);
        y -= char_height;
    }
}

void glyph_animation::draw() const
{
    render::push_matrix();
    render::translate(x_base, y_base);
    render::scale(scale, scale);

    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    for (const auto &p : glyph_states) {
        const float a = alpha * p.glyph_alpha;
        p.draw(program_, a);
    }

    render::pop_matrix();
}

glyph_animation::glyph_state::glyph_state(const g2d::glyph_info *gi, bool flip, float x_center, float y_center,
                                          const gradient& g)
    : texture(gi->texture_)
    , x_center(x_center)
    , y_center(y_center)
{
    if (flip) {
        const float dx = .5 * gi->height;
        const float dy = .5 * gi->width;

        p0 = g2d::vec2(dx, dy);
        p1 = g2d::vec2(dx, -dy);
        p2 = g2d::vec2(-dx, -dy);
        p3 = g2d::vec2(-dx, dy);
    } else {
        const float dx = .5 * gi->width;
        const float dy = .5 * gi->height;

        p0 = g2d::vec2(-dx, dy);
        p1 = g2d::vec2(dx, dy);
        p2 = g2d::vec2(dx, -dy);
        p3 = g2d::vec2(-dx, -dy);
    }

    t0 = gi->texuv[0];
    t1 = gi->texuv[1];
    t2 = gi->texuv[2];
    t3 = gi->texuv[3];

    top_color_text = g.from;
    bottom_color_text = g.to;

    top_color_outline = .5 * top_color_text;
    bottom_color_outline = .5 * bottom_color_text;
}

void glyph_animation::glyph_state::draw(const g2d::program *program, float alpha) const
{
    const float c = sin(flip);
    const float scale = 1. / (1. + z);

    render::push_matrix();
    render::translate(x_center, y_center);
    render::scale(c * scale, scale);

    const g2d::rgba top_outline{top_color_outline, alpha};
    const g2d::rgba bottom_outline{bottom_color_outline, alpha};
    const g2d::rgba top_text{top_color_text, alpha};
    const g2d::rgba bottom_text{bottom_color_text, alpha};

    render::draw_quad(program, texture, {{p0.x, p0.y}, {p1.x, p1.y}, {p2.x, p2.y}, {p3.x, p3.y}},
                      {{t0.x, t0.y}, {t1.x, t1.y}, {t2.x, t2.y}, {t3.x, t3.y}},
                      {top_outline, top_outline, bottom_outline, bottom_outline},
                      {top_text, top_text, bottom_text, bottom_text},
                      100);

    render::pop_matrix();
}

#define GLYPH_ANIMATION_SPACING 1.

level_intro_animation::level_intro_animation(const gradient& g)
    : glyph_animation(get_font(font::title), GLYPH_ANIMATION_SPACING, L"用意",
                      .5 * window_width, .5 * window_height, g)
{
    const size_t num_glyphs = glyph_states.size();

    auto *p = new parallel_action_group;

    for (size_t i = 0; i < num_glyphs; i++) {
        p->add((new sequential_action_group)
                   ->add(new delay_action(i * 15 * MS_PER_TIC))
                   ->add(new property_change_action<out_bounce_tween<float>>(&glyph_states[i].flip, 0, .5 * M_PI,
                                                                             30 * MS_PER_TIC)));
    }

    // bump
    p->add((new sequential_action_group)
               ->add(new delay_action(45 * MS_PER_TIC))
               ->add(new property_change_action<in_cos_tween<float>>(&scale, 1., 1.2, 10 * MS_PER_TIC))
               ->add(new property_change_action<out_bounce_tween<float>>(&scale, 1.2, 1, 20 * MS_PER_TIC)));

    // fade
    p->add((new sequential_action_group)
               ->add(new delay_action((num_glyphs * 30 + 20) * MS_PER_TIC))
               ->add(new property_change_action<quadratic_tween<float>>(&alpha, 1, 0, 10 * MS_PER_TIC)));

    action.reset(p);

    for (auto &p : glyph_states) {
        p.flip = 0;
        p.z = 0;
        p.glyph_alpha = 1;
    }
}

level_completed_animation::level_completed_animation(const gradient& g)
    : glyph_animation(get_font(font::title), GLYPH_ANIMATION_SPACING, L"成功",
                      .5 * window_width, .5 * window_height, g)
{
    auto *p = new parallel_action_group;

    for (size_t i = 0; i < glyph_states.size(); i++) {
        p->add((new sequential_action_group)
                   ->add(new delay_action(i * 15 * MS_PER_TIC))
                   ->add((new parallel_action_group)
                             ->add(new property_change_action<out_bounce_tween<float>>(&glyph_states[i].z, -.7, 0,
                                                                                       40 * MS_PER_TIC))
                             ->add(new property_change_action<quadratic_tween<float>>(&glyph_states[i].glyph_alpha, 0,
                                                                                      1, 20 * MS_PER_TIC))));
    }

    const int FADE_TICS = 10 * MS_PER_TIC;

    p->add((new sequential_action_group)
               ->add(new delay_action(LEVEL_COMPLETED_TICS - FADE_TICS))
               ->add(new property_change_action<quadratic_tween<float>>(&alpha, 1, 0, FADE_TICS)));

    action.reset(p);

    for (auto &p : glyph_states) {
        p.flip = .5 * M_PI;
        p.glyph_alpha = 0;
        p.z = 2;
    }
}

abunai_animation::abunai_animation()
    : x(window_width)
    , alpha(0)
    , abunai_bb_(g2d::get_sprite("abunai.png"))
{
    const float w = abunai_bb_->get_width();

    action.reset(
        (new parallel_action_group)
            ->add((new sequential_action_group)
                      ->add(new delay_action(10 * MS_PER_TIC))
                      ->add(new property_change_action<out_bounce_tween<float>>(&x, window_width, 40, 40 * MS_PER_TIC))
                      ->add(new delay_action(45 * MS_PER_TIC))
                      ->add(new property_change_action<in_back_tween<float>>(&x, 40, -w, 30 * MS_PER_TIC)))
            ->add((new sequential_action_group)
                      ->add(new property_change_action<quadratic_tween<float>>(&alpha, 0, .8, 50 * MS_PER_TIC))
                      ->add(new delay_action(75 * MS_PER_TIC))
                      ->add(new property_change_action<quadratic_tween<float>>(&alpha, .8, 0, 10 * MS_PER_TIC))));
}

void abunai_animation::draw() const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    // draw fade out overlay

    render::set_color({0, 0, 0, alpha});
    render::draw_quad({{0, 0}, {0, window_height}, {window_width, window_height}, {window_width, 0}}, 99);

    // draw billboard

    const float scale = window_width / abunai_bb_->get_width();

    render::set_color({1, 1, 1, 1});
    render::push_matrix();
    render::translate(x, 0);
    render::scale(scale, scale);
    abunai_bb_->draw(0, 0, 100);
    render::pop_matrix();
}

game_over_animation::game_over_animation(const g2d::font *font, float spacing, const wchar_t *str,
                                                   const gradient& g)
    : glyph_animation(font, spacing, str, .2 * window_width, .5 * window_height, g)
    , billboard_x(window_width)
    , overlay_alpha(0)
    , game_over_bb_(g2d::get_sprite("h-oh.png"))
{
    auto *p = new parallel_action_group;

    // overlay alpha
    p->add(new property_change_action<quadratic_tween<float>>(&overlay_alpha, 0, .8, 50 * MS_PER_TIC));

    // glyphs
    for (size_t i = 0; i < glyph_states.size(); i++) {
        p->add((new sequential_action_group)
                   ->add(new delay_action((30 + i * 15) * MS_PER_TIC))
                   ->add(new property_change_action<out_bounce_tween<float>>(&glyph_states[i].flip, 0, .5 * M_PI,
                                                                             30 * MS_PER_TIC)));
    }

    // billboard
    p->add((new sequential_action_group)
               ->add(new delay_action(10))
               ->add(new property_change_action<out_bounce_tween<float>>(&billboard_x, window_width, 40,
                                                                         40 * MS_PER_TIC)));

    p->add(new delay_action(300 * MS_PER_TIC));

    action.reset(p);

    for (auto &p : glyph_states) {
        p.flip = 0;
        p.z = 0;
        p.glyph_alpha = 1;
    }
}

void game_over_animation::draw() const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    // draw fade out overlay

    render::set_color({0, 0, 0, overlay_alpha});
    render::draw_quad({{0, 0}, {0, window_height}, {window_width, window_height}, {window_width, 0}}, 99);

    // draw billboard

    const float scale = window_width / game_over_bb_->get_width();

    render::set_color({1, 1, 1, 1});
    render::push_matrix();
    render::translate(billboard_x, 0);
    render::scale(scale, scale);
    game_over_bb_->draw(0, 0, 100);
    render::pop_matrix();

    // draw characters

    glyph_animation::draw();
}

countdown_digit::countdown_digit(const g2d::glyph_info *gi, float x_center, float y_center)
    : texture(gi->texture_)
    , x_center(x_center)
    , y_center(y_center)
    , z(-.7)
    , alpha(0)
{
    const float dx = .5 * gi->width;
    const float dy = .5 * gi->height;
    verts = {{-dx, dy}, {dx, dy}, {dx, -dy}, {-dx, -dy}};

    texcoords = {gi->texuv[0], gi->texuv[1], gi->texuv[2], gi->texuv[3]};

    action.reset((new parallel_action_group)
                 ->add((new sequential_action_group)
                           ->add(new property_change_action<out_bounce_tween<float>>(&z, -.7, -.1, 15 * MS_PER_TIC))
                           ->add(new delay_action(10 * MS_PER_TIC))
                           ->add(new property_change_action<quadratic_tween<float>>(&z, -.1, .3, 5 * MS_PER_TIC)))
                 ->add((new sequential_action_group)
                           ->add(new property_change_action<quadratic_tween<float>>(&alpha, 0, .6, 5 * MS_PER_TIC))
                           ->add(new delay_action(20 * MS_PER_TIC))
                           ->add(new property_change_action<quadratic_tween<float>>(&alpha, .6, 0, 5 * MS_PER_TIC))));
}

void countdown_digit::draw() const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    const float scale = 1. / (1. + z);

    render::set_color({1, 0, 0, alpha});
    render::push_matrix();
    render::translate(x_center, y_center);
    render::scale(scale, scale);

    render::draw_quad(texture, verts, texcoords, 120);

    render::pop_matrix();
}

class in_game_state_impl : public world_event_listener
{
public:
    in_game_state_impl();

    void reset();
    void redraw() const;
    void update(uint32_t dt);
    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_touch_move(float x, float y);
    void on_back_key();
    void on_menu_key();

    void set_jukugo_left(int jukugo_left) override;
    void set_score(int score) override;
    void set_next_falling_blocks(wchar_t left, wchar_t right) override;
    void show_hint(const jukugo *hint);
    void on_falling_blocks_started() override;

    void set_enable_hints(bool enable);

private:
    void draw_hud() const;
    float get_level_transition_alpha() const;

    enum game_state
    {
        STATE_LEVEL_INTRO,
        STATE_IN_GAME,
        STATE_GAME_OVER,
        STATE_LEVEL_COMPLETED,
        STATE_ABUNAI,
    };

    void set_state(game_state next_state);
    void set_cur_game_animation(game_animation *p);
    void timer_update();
    void reset_level();
    void bump_level();
    void finish_level();
    void set_state_game_over(bool time_up);
    void finish_in_game();

    world world_;
    int state_tics_;
    std::unique_ptr<game_animation> cur_game_animation_;
    pause_button pause_button_;
    timer_display timer_display_;
    score_display score_display_;

    bool abunai_shown_;
    int last_countdown_digit_;

    bool touch_is_down_;
    float start_touch_x_, start_touch_y_;
    bool gesture_started_;
    int touch_down_tics_;

    enum gesture
    {
        GESTURE_NONE,
        GESTURE_TAP,
        GESTURE_SWIPE_LEFT,
        GESTURE_SWIPE_RIGHT,
        GESTURE_SWIPE_DOWN
    };
    gesture enqueued_gesture_;

    game_state cur_state_;

    wchar_t next_falling_blocks_[3];

    std::unique_ptr<theme_animation> theme_;
    const color_scheme *colors_;
};

in_game_state_impl::in_game_state_impl()
    : world_(GRID_ROWS, GRID_COLS, window_height - 120)
    , cur_game_animation_(nullptr)
    , pause_button_(window_width - pause_button::SIZE, window_height - pause_button::SIZE)
    , touch_is_down_(false)
{
    pause_button_.set_callback([this] { on_back_key(); });

    world_.set_event_listener(this);

    grid_base_x = .5 * (window_width - world_.get_width());
    grid_base_y = .5 * (window_height - world_.get_height()) - 20;

    memset(next_falling_blocks_, 0, sizeof next_falling_blocks_);
}

float in_game_state_impl::get_level_transition_alpha() const
{
    if (cur_state_ == STATE_LEVEL_COMPLETED) {
        if (state_tics_ >= LEVEL_COMPLETED_TICS - FADE_OUT_TICS) {
            float alpha = 1. - (float)(state_tics_ - (LEVEL_COMPLETED_TICS - FADE_OUT_TICS)) / FADE_OUT_TICS;

            if (alpha < 0)
                alpha = 0;

            return alpha * alpha;
        }
    } else if (cur_state_ == STATE_LEVEL_INTRO) {
        if (state_tics_ < FADE_IN_TICS) {
            float alpha = (float)state_tics_ / FADE_IN_TICS;

            return alpha * alpha;
        }
    }

    return 1.;
}

void in_game_state_impl::set_state(game_state next_state)
{
    cur_state_ = next_state;
    state_tics_ = 0;
}

void in_game_state_impl::set_cur_game_animation(game_animation *p)
{
    cur_game_animation_.reset(p);
}

void in_game_state_impl::reset_level()
{
    int theme_index = cur_level % NUM_THEMES;

    theme_ = make_theme(theme_index);
    colors_ = &cur_settings.color_schemes[theme_index];

    theme_->reset();

    world_.set_level(cur_level, practice_mode, cur_options->enable_hints);

    timer_display_.reset(cur_settings.game.level_secs * 1000);

#if 1
    set_state(STATE_LEVEL_INTRO);
    set_cur_game_animation(new level_intro_animation(colors_->text_gradient));
#else
    set_state(STATE_IN_GAME);
#endif

    world_.set_theme_colors(colors_->main_color, colors_->alternate_color);
    world_.set_text_gradient(colors_->text_gradient);

    const auto& gradient = colors_->background_gradient;
    background_initialize_gradient(gradient.from, gradient.to);

    if (!practice_mode && cur_level > cur_options->max_unlocked_level)
        cur_options->max_unlocked_level = cur_level;

    abunai_shown_ = false;
    last_countdown_digit_ = 9;
    touch_is_down_ = false;

    start_sound(SOUND_LEVEL_INTRO, false);
}

void in_game_state_impl::bump_level()
{
    ++cur_level;
    reset_level();
}

void in_game_state_impl::finish_level()
{
    set_state(STATE_LEVEL_COMPLETED);
    start_sound(SOUND_LEVEL_COMPLETED, false);
    set_cur_game_animation(new level_completed_animation(colors_->text_gradient));
}

void in_game_state_impl::set_state_game_over(bool time_up)
{
    set_state(STATE_GAME_OVER);
    start_sound(SOUND_GAME_OVER, false);

    game_animation *animation;

    if (time_up) {
        world_.set_game_over();
        animation = new game_over_animation(get_font(font::title), 1.1, L"時間切れ", colors_->text_gradient);
    } else {
        animation = new game_over_animation(get_font(font::gameover), 1.1, L"げ〜むお〜ば〜", colors_->text_gradient);
    }

    set_cur_game_animation(animation);
}

void in_game_state_impl::finish_in_game()
{
    auto *main_menu = static_cast<main_menu_state *>(get_prev_state());

    if (practice_mode) {
        main_menu->show_background();
        main_menu->show_root_menu();
        main_menu->fade_in();
        pop_state();
    } else {
        // oh god...
        pop_state();
        main_menu->fade_in();
        start_hiscore_input();
        static_cast<hiscore_input_state *>(get_cur_state())->set_score(world_.get_score());
    }
}

void in_game_state_impl::update(uint32_t dt)
{
    state_tics_ += dt;
    total_tics += dt;

    if (touch_is_down_)
        touch_down_tics_ += dt;

    theme_->update(dt);

    score_display_.update(dt);

    bool animation_done = false;

    if (cur_game_animation_) {
        if (!cur_game_animation_->update(dt)) {
            if (cur_state_ != STATE_GAME_OVER) {
                set_cur_game_animation(nullptr);
                animation_done = true;
            } else {
                finish_in_game();
            }
        }
    }

    if (!abunai_shown_ &&
        /* cur_state_ == STATE_FALLING_BLOCK && */ timer_display_.blinking()) {
        set_state(STATE_ABUNAI);
        set_cur_game_animation(new abunai_animation);
        abunai_shown_ = true;
    }

    switch (cur_state_) {
        case STATE_GAME_OVER:
            world_.update_animations(dt); // animate sprites etc
            break;

        case STATE_ABUNAI:
        case STATE_LEVEL_INTRO:
            if (animation_done)
                set_state(STATE_IN_GAME);
            break;

        case STATE_LEVEL_COMPLETED:
            if (animation_done)
                bump_level();
            break;

        case STATE_IN_GAME:
            if (dpad_state & DPAD_LEFT)
                world_.on_left_pressed();

            if (dpad_state & DPAD_RIGHT)
                world_.on_right_pressed();

            if (dpad_state & DPAD_UP)
                world_.on_up_pressed();

            if (dpad_state & DPAD_DOWN)
                world_.on_down_pressed();

            if (enqueued_gesture_ != GESTURE_NONE) {
                bool consumed = false;

                switch (enqueued_gesture_) {
                    case GESTURE_TAP:
                        consumed = world_.on_up_pressed();
                        break;

                    case GESTURE_SWIPE_LEFT:
                        consumed = world_.on_left_pressed();
                        break;

                    case GESTURE_SWIPE_RIGHT:
                        consumed = world_.on_right_pressed();
                        break;

                    case GESTURE_SWIPE_DOWN:
                        consumed = world_.on_down_pressed();
                        break;

                    default:
                        break;
                }

                if (consumed)
                    enqueued_gesture_ = GESTURE_NONE;
            }

            world_.update(dt);

            if (world_.is_in_game_over_state()) {
                set_state_game_over(false);
            } else if (world_.is_in_level_completed_state()) {
                finish_level();
            } else if (world_.is_in_falling_block_state()) {
                if (!practice_mode) {
                    assert(!timer_display_.finished());

                    timer_display_.update(dt);

                    if (timer_display_.finished()) {
                        set_state_game_over(true);
                    } else if (timer_display_.get_tics_left() / 1000 < last_countdown_digit_) {
                        const auto *f = get_font(font::large);
                        set_cur_game_animation(new countdown_digit(f->find_glyph(L'0' + last_countdown_digit_),
                                                                   grid_base_x + .5 * world_.get_width(),
                                                                   grid_base_y + .5 * world_.get_height()));
                        start_sound(SOUND_COUNTDOWN, false);

                        --last_countdown_digit_;
                    }
                }
            }
            break;

        default:
            assert(0);
    }
}

void in_game_state_impl::draw_hud() const
{
    const float alpha = get_level_transition_alpha();

    const float grid_width = world_.get_width();
    const float grid_height = world_.get_height();

    const auto *tiny_font = get_font(font::tiny);
    const auto *medium_font = get_font(font::medium);

    // no alpha

    render::set_blend_mode(blend_mode::ALPHA_BLEND);
    render::set_color({1.f, 1.f, 1.f, 1.f});

    render::push_matrix();
    render::translate(grid_base_x + grid_width - 6, grid_base_y + grid_height + 60);
    render::scale(.7f, .7f);
    render::set_text_align(text_align::RIGHT);
    render::draw_text(tiny_font, {}, 50, L"Score");
    render::pop_matrix();

    render::push_matrix();
    render::translate(grid_base_x, grid_base_y - 28);
    {
        std::wstringstream ss;
        ss << L"Level " << cur_level + 1;
        render::set_text_align(text_align::LEFT);
        render::draw_text(tiny_font, {}, 50, ss.str().c_str());
    }
    render::pop_matrix();

    // alpha
    render::set_color({1.f, 1.f, 1.f, alpha});

    // jukugo left label
    render::push_matrix();
    render::translate(grid_base_x + grid_width - 8, grid_base_y - 28);

    render::set_text_align(text_align::RIGHT);
    if (!practice_mode) {
        std::wstringstream ss;
        ss << L"Left:" << world_.get_jukugo_left();
        render::draw_text(tiny_font, {}, 50, ss.str().c_str());
    } else {
        render::draw_text(tiny_font, {}, 50, L"practice");
    }

    render::pop_matrix();

    // "Next"
    render::push_matrix();
    render::translate(grid_base_x, grid_base_y + grid_height + 60);
    render::scale(.7f, .7f);
    render::set_text_align(text_align::LEFT);
    render::draw_text(tiny_font, {}, 50, L"Next");
    render::pop_matrix();

    // next jukugo
    render::push_matrix();
    render::translate(grid_base_x, grid_base_y + grid_height + 16);
    render::scale(.8f, .8f);
    render::draw_text(medium_font, {}, 50, next_falling_blocks_);
    render::pop_matrix();

    score_display_.draw(grid_base_x + world_.get_width() - 8, grid_base_y + world_.get_height() + 28);

    if (!practice_mode) {
        render::push_matrix();
        render::translate(0, grid_base_y + world_.get_height() + 32);
        timer_display_.draw(alpha);
        render::pop_matrix();
    }
}

void in_game_state_impl::redraw() const
{
    // theme colors

    g2d::rgb theme_color, theme_opposite_color;

    if (cur_state_ == STATE_LEVEL_COMPLETED || cur_state_ == STATE_LEVEL_INTRO) {
        const float alpha = get_level_transition_alpha();
        theme_color = colors_->main_color * alpha + g2d::rgb(1.f, 1.f, 1.f) * (1. - alpha);
        theme_opposite_color = colors_->alternate_color * alpha + g2d::rgb(1.f, 1.f, 1.f) * (1. - alpha);
    } else {
        theme_color = colors_->main_color;
        theme_opposite_color = colors_->alternate_color;
    }

    const_cast<world *>(&world_)->set_theme_colors(theme_color, theme_opposite_color);

    background_draw_gradient();

    theme_->draw();

    if (cur_state_ == STATE_LEVEL_COMPLETED || cur_state_ == STATE_LEVEL_INTRO)
        draw_fade_overlay(1. - get_level_transition_alpha());

    pause_button_.draw(1);

    draw_hud();

    render::push_matrix();
    render::translate(grid_base_x, grid_base_y);
    world_.draw();
    render::pop_matrix();

    if (cur_game_animation_)
        cur_game_animation_->draw();
}

void in_game_state_impl::reset()
{
    set_cur_game_animation(nullptr);

    touch_is_down_ = false;

    total_tics = 0;

    pause_button_.reset();

    score_display_.reset();

    world_.reset();
    reset_level();
    world_.initialize_grid(2);

    enqueued_gesture_ = GESTURE_NONE;
}

void in_game_state_impl::on_touch_down(float x, float y)
{
    start_touch_x_ = x;
    start_touch_y_ = y;

    gesture_started_ = false;

    touch_is_down_ = true;
    touch_down_tics_ = 0;

    if (cur_state_ == STATE_GAME_OVER) {
        if (state_tics_ > 100)
            finish_in_game();
        return;
    }

    if (pause_button_.on_touch_down(x, y))
        return;
}

enum
{
    TAP_MAX_TICS = 12 * MS_PER_TIC,
};

void in_game_state_impl::on_touch_up()
{
    if (!touch_is_down_)
        return;

    touch_is_down_ = false;

    if (pause_button_.on_touch_up())
        return;

    if (cur_state_ == STATE_IN_GAME && world_.can_consume_gestures()) {
        // gesture controls

        if (!gesture_started_ && touch_down_tics_ < TAP_MAX_TICS) {
            if (!world_.on_up_pressed())
                enqueued_gesture_ = GESTURE_TAP;
        }
    }
}

void in_game_state_impl::on_touch_move(float x, float y)
{
    if (!touch_is_down_)
        return;

    if (pause_button_.on_touch_down(x, y))
        return;

    if (cur_state_ == STATE_IN_GAME && world_.can_consume_gestures()) {
        // gesture controls

        const float SWIPE_HORIZ_THRESHOLD = .5 * world_.get_cell_size();
        const float SWIPE_DOWN_THRESHOLD = world_.get_cell_size();

        const float dx = x - start_touch_x_;
        const float dy = y - start_touch_y_;

        if (fabs(dx) > SWIPE_HORIZ_THRESHOLD) {
            // swipe left/right

            if (dx < 0) {
                if (!world_.on_left_pressed())
                    enqueued_gesture_ = GESTURE_SWIPE_LEFT;
            } else {
                if (!world_.on_right_pressed())
                    enqueued_gesture_ = GESTURE_SWIPE_RIGHT;
            }

            touch_is_down_ = false;
        } else if (dy < -SWIPE_DOWN_THRESHOLD) {
            // swipe down

            if (dy < 0) {
                if (!world_.on_down_pressed())
                    enqueued_gesture_ = GESTURE_SWIPE_DOWN;
            }

            touch_is_down_ = false;
        }
    }
}

void in_game_state_impl::on_back_key()
{
    if (cur_state_ != STATE_GAME_OVER)
        start_in_game_menu();
}

void in_game_state_impl::on_menu_key()
{
    on_back_key();
}

void in_game_state_impl::set_jukugo_left(int jukugo_left)
{
}

void in_game_state_impl::set_score(int score)
{
    score_display_.enqueue_value(score);
}

void in_game_state_impl::set_next_falling_blocks(const wchar_t left, const wchar_t right)
{
    next_falling_blocks_[0] = left;
    next_falling_blocks_[1] = right;
}

void in_game_state_impl::on_falling_blocks_started()
{
    enqueued_gesture_ = GESTURE_NONE;
}

void in_game_state_impl::set_enable_hints(bool enable)
{
    world_.set_enable_hints(enable);
}

in_game_state::in_game_state()
    : impl_(new in_game_state_impl)
{
}

in_game_state::~in_game_state()
{
    delete impl_;
}

void in_game_state::reset()
{
    impl_->reset();
}

void in_game_state::redraw() const
{
    impl_->redraw();
}

void in_game_state::update(uint32_t dt)
{
    impl_->update(dt);
}

void in_game_state::on_touch_down(float x, float y)
{
    impl_->on_touch_down(x, y);
}

void in_game_state::on_touch_up()
{
    impl_->on_touch_up();
}

void in_game_state::on_touch_move(float x, float y)
{
    impl_->on_touch_move(x, y);
}

void in_game_state::on_back_key()
{
    impl_->on_back_key();
}

void in_game_state::on_menu_key()
{
    impl_->on_menu_key();
}

void in_game_state::set_enable_hints(bool enable)
{
    impl_->set_enable_hints(enable);
}
