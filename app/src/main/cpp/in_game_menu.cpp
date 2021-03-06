#include <cassert>
#include <cstdio>

#include "guava2d/rgb.h"
#include "guava2d/texture_manager.h"

#include "common.h"
#include "in_game.h"
#include "in_game_menu.h"
#include "main_menu.h"
#include "menu.h"
#include "options.h"
#include "render.h"
#include "sounds.h"
#include "sprite_manager.h"

namespace {

class vertical_menu : public menu
{
private:
    static constexpr int ITEM_WIDTH = 280;
    static constexpr int ITEM_HEIGHT = 80;

    g2d::vec2 get_item_position(int item_index) const override;
};

g2d::vec2 vertical_menu::get_item_position(int item_index) const
{
    const float total_height = item_list_.size() * ITEM_HEIGHT;

    const auto &item = item_list_[item_index];

    const float base_y = .5 * (window_height + total_height) - .5 * ITEM_HEIGHT;
    const float base_x = .5 * (window_width - ITEM_WIDTH);

    float y = base_y - item_index * ITEM_HEIGHT - .5 * ITEM_HEIGHT;

    float offset_scale = 1. - get_cur_alpha();
    float x_offset = (.5 * window_width + item_index * 72.) * offset_scale * offset_scale;

    float x = base_x + x_offset - 32. * sinf(item->get_active_t() * M_PI);

    return g2d::vec2(x, y);
}

} // namespace

class in_game_menu_impl
{
public:
    in_game_menu_impl();

    void reset();
    void redraw() const;
    void update(uint32_t dt);
    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_touch_move(float x, float y);
    void on_back_key();
    void on_menu_key();

private:
    void create_root_menu();
    void create_options_menu();

    void set_cur_menu(menu *p);

    enum state
    {
        STATE_FADE_IN,
        STATE_ACTIVE,
        STATE_FADE_OUT,
        STATE_FADE_TO_JUKUGO_STATS,
    };
    void set_cur_state(state s);

    void draw_frame(float alpha) const;

    vertical_menu root_menu_, options_menu_;
    menu *cur_menu_;
    bool touch_is_down_;

    static constexpr int FRAME_HEIGHT = 340;
    static constexpr int FADE_IN_TICS = 10 * MS_PER_TIC;

    state cur_state_;
    int state_tics_;
};

in_game_menu_impl::in_game_menu_impl()
{
    create_root_menu();
    create_options_menu();

    reset();
}

void in_game_menu_impl::create_root_menu()
{
    root_menu_.append_action_item(SOUND_MENU_VALIDATE, "resume-0.png", "resume-1.png")
        .set_on_clicked([this] { set_cur_state(STATE_FADE_OUT); })
        .set_action(pop_state)
        .set_is_back(true);

    root_menu_.append_action_item(SOUND_MENU_SELECT, "options-0.png", "options-1.png").set_action([this] {
        set_cur_menu(&options_menu_);
    });

    root_menu_.append_action_item(SOUND_MENU_SELECT, "stats-0.png", "stats-1.png")
        .set_on_clicked([this] { set_cur_state(STATE_FADE_TO_JUKUGO_STATS); })
        .set_action([] {
            // i'm going to regret this some day i know it
            extern ::state *get_main_menu_state();
            static_cast<main_menu_state *>(get_main_menu_state())->unfade();
            start_stats_page();
        });

    root_menu_.append_action_item(SOUND_MENU_BACK, "quit-0.png", "quit-1.png")
        .set_on_clicked([this] { set_cur_state(STATE_FADE_OUT); })
        .set_action([] {
            pop_state();
            pop_state();
            get_cur_state()->reset();
        });
}

void in_game_menu_impl::create_options_menu()
{
    options_menu_
        .append_toggle_item(SOUND_MENU_SELECT, cur_options->enable_hints, "hints-on-0.png", "hints-on-1.png",
                            "hints-off-0.png", "hints-off-1.png")
        .set_on_toggle([](int enable_hints) {
            extern ::state *get_in_game_state();
            static_cast<in_game_state *>(get_in_game_state())->set_enable_hints(enable_hints);
        });

    options_menu_
        .append_toggle_item(SOUND_MENU_SELECT, cur_options->enable_sound, "sound-on-0.png", "sound-on-1.png",
                            "sound-off-0.png", "sound-off-1.png")
        .set_on_toggle([](int enable_sound) {
            if (!enable_sound)
                stop_all_sounds();
            else
                start_sound(SOUND_MENU_SELECT, false);
        });

    options_menu_.append_action_item(SOUND_MENU_BACK, "back-0.png", "back-1.png")
        .set_action([this] { set_cur_menu(&root_menu_); })
        .set_is_back(true);
}

void in_game_menu_impl::reset()
{
    touch_is_down_ = false;

    set_cur_menu(&root_menu_);
    set_cur_state(STATE_FADE_IN);
}

void in_game_menu_impl::set_cur_state(state s)
{
    cur_state_ = s;
    state_tics_ = 0;
}

void in_game_menu_impl::set_cur_menu(menu *p)
{
    cur_menu_ = p;
    cur_menu_->reset();
}

void in_game_menu_impl::draw_frame(float alpha) const
{
    alpha *= 1.5;
    if (alpha > 1)
        alpha = 1;

    const float a = alpha * .75;

    const float y0 = .5 * (window_height - FRAME_HEIGHT);
    const float y1 = .5 * (window_height + FRAME_HEIGHT);

    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    // top/bottom

    render::set_color({0.f, 0.f, 0.f, a});

    // bottom (black)

    render::draw_quad({{0, 0}, {window_width, 0}, {window_width, y0}, {0, y0}}, 20);

    // top (black)

    render::draw_quad({{0, y1}, {window_width, y1}, {window_width, window_height}, {0, window_height}}, 20);

    // center (white)

    render::set_color({.75f, .75f, .75f, a});

    render::draw_quad({{0, y0}, {window_width, y0}, {window_width, y1}, {0, y1}}, 20);
}

void in_game_menu_impl::redraw() const
{
    get_prev_state()->redraw();

    float alpha;

    switch (cur_state_) {
        case STATE_FADE_IN:
            alpha = static_cast<float>(state_tics_) / FADE_IN_TICS;
            break;

        case STATE_FADE_OUT:
            alpha = cur_menu_->get_cur_alpha();
            break;

        default:
            alpha = 1;
            break;
    }

    draw_frame(alpha);

    cur_menu_->draw();

    if (cur_state_ == STATE_FADE_TO_JUKUGO_STATS)
        draw_fade_overlay(1. - cur_menu_->get_cur_alpha());
}

void in_game_menu_impl::update(uint32_t dt)
{
    if (cur_state_ == STATE_FADE_IN) {
        if ((state_tics_ += dt) >= FADE_IN_TICS)
            set_cur_state(STATE_ACTIVE);
    } else {
        cur_menu_->update(dt);
    }
}

void in_game_menu_impl::on_touch_down(float x, float y)
{
    touch_is_down_ = true;
    cur_menu_->on_touch_down(x, y);
}

void in_game_menu_impl::on_touch_up()
{
    touch_is_down_ = false;
    cur_menu_->on_touch_up();
}

void in_game_menu_impl::on_touch_move(float x, float y)
{
    if (touch_is_down_)
        cur_menu_->on_touch_down(x, y);
}

void in_game_menu_impl::on_back_key()
{
    cur_menu_->on_back_key();
}

void in_game_menu_impl::on_menu_key()
{
}

in_game_menu_state::in_game_menu_state()
    : impl_(new in_game_menu_impl)
{
}

in_game_menu_state::~in_game_menu_state()
{
    delete impl_;
}

void in_game_menu_state::reset()
{
    impl_->reset();
}

void in_game_menu_state::redraw() const
{
    impl_->redraw();
}

void in_game_menu_state::update(uint32_t dt)
{
    impl_->update(dt);
}

void in_game_menu_state::on_touch_down(float x, float y)
{
    impl_->on_touch_down(x, y);
}

void in_game_menu_state::on_touch_up()
{
    impl_->on_touch_up();
}

void in_game_menu_state::on_touch_move(float x, float y)
{
    impl_->on_touch_move(x, y);
}

void in_game_menu_state::on_back_key()
{
    impl_->on_back_key();
}

void in_game_menu_state::on_menu_key()
{
    impl_->on_menu_key();
}
