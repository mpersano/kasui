#include <sstream>

#include "render.h"

#include "common.h"
#include "main_menu.h"
#include "menu.h"
#include "options.h"
#include "sounds.h"
#include "title_background.h"

extern void on_rate_me_clicked();

class vertical_menu : public menu
{
private:
    static constexpr int ITEM_WIDTH = 280;
    static constexpr int ITEM_HEIGHT = 80;

    g2d::vec2 get_item_position(int item_index) const override;
};

class level_selection_menu : public menu
{
public:
    level_selection_menu(main_menu_impl *parent);

private:
    static constexpr int NUM_BUTTON_COLS = 3;
    static constexpr int NUM_BUTTON_ROWS = (NUM_LEVELS + NUM_BUTTON_COLS - 1) / NUM_BUTTON_COLS;

    static constexpr int ITEM_WIDTH = 200;
    static constexpr int ITEM_HEIGHT = 200;
    static constexpr int BACK_BUTTON_WIDTH = 180;
    static constexpr int BACK_BUTTON_HEIGHT = 80;

    g2d::vec2 get_item_position(int item_index) const override;
};

class main_menu_impl
{
public:
    main_menu_impl();
    ~main_menu_impl();

    void reset();
    void redraw() const;
    void update(uint32_t dt);
    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_touch_move(float x, float y);
    void on_back_key();
    void on_menu_key();

    void show_root_menu();
    void show_more_menu();
    void show_game_mode_menu();
    void show_background();
    void hide_background();
    void fade_in();

    void set_state_outro();
    void set_state_active();

private:
    void create_main_menu();
    void create_more_menu();
    void create_options_menu();
    void create_game_mode_menu();

    void set_cur_menu(menu *p);

    title_background background_;

    vertical_menu main_menu_;
    vertical_menu options_menu_;
    vertical_menu more_menu_;
    vertical_menu game_mode_menu_;
    level_selection_menu level_menu_;

    static constexpr int FADE_IN_T = 10 * MS_PER_TIC;

    menu *cur_menu_;

    enum class state
    {
        FADE_IN,
        ACTIVE,
        OUTRO,
    } cur_state_;

    uint32_t state_t_;
    bool touch_is_down_ = false;
};

main_menu_impl::main_menu_impl()
    : level_menu_(this)
{
    create_main_menu();
    create_options_menu();
    create_more_menu();
    create_game_mode_menu();
}

main_menu_impl::~main_menu_impl() = default;

void main_menu_impl::set_cur_menu(menu *p)
{
    cur_menu_ = p;
    cur_menu_->reset();
}

void main_menu_impl::show_root_menu()
{
    set_cur_menu(&main_menu_);
}

void main_menu_impl::show_background()
{
    background_.show_billboard();
    background_.show_logo();
}

void main_menu_impl::hide_background()
{
    background_.hide_billboard();
    background_.hide_logo();
}

void main_menu_impl::show_game_mode_menu()
{
    set_cur_menu(&game_mode_menu_);
}

void main_menu_impl::show_more_menu()
{
    set_cur_menu(&more_menu_);
}

void main_menu_impl::set_state_outro()
{
    cur_state_ = state::OUTRO;
}

void main_menu_impl::set_state_active()
{
    cur_state_ = state::ACTIVE;
}

void main_menu_impl::fade_in()
{
    cur_state_ = state::FADE_IN;
    state_t_ = 0;
}

g2d::vec2 vertical_menu::get_item_position(int item_index) const
{
    const float total_height = item_list_.size() * ITEM_HEIGHT;

    const auto &item = item_list_[item_index];

    const float base_y = .5 * (window_height + total_height) - .5 * ITEM_HEIGHT;
    const float base_x = window_width - ITEM_WIDTH;

    const float y = base_y - item_index * ITEM_HEIGHT - .5 * ITEM_HEIGHT;

    const float offset_scale = 1. - get_cur_alpha();
    const float x_offset = (.5 * window_width + item_index * 72.) * offset_scale * offset_scale;

    const float x = base_x + x_offset - 32. * sinf(item->get_active_t() * M_PI);

    return {x, y};
}

class level_menu_item : public action_menu_item
{
public:
    level_menu_item(main_menu_impl *parent, int level_id)
        : action_menu_item(SOUND_MENU_VALIDATE, level_sprite_name(level_id * 2), level_sprite_name(level_id * 2 + 1))
        , level_id_(level_id)
    {
        set_on_clicked([parent] { parent->set_state_outro(); });
        set_action([level_id] { start_in_game(level_id); });
    }

    bool is_enabled() const override { return practice_mode || level_id_ <= cur_options->max_unlocked_level; }

private:
    static std::string level_sprite_name(int id)
    {
        std::stringstream ss;
        ss << "level-" << id << ".png";
        return ss.str();
    }

    int level_id_;
};

level_selection_menu::level_selection_menu(main_menu_impl *parent)
{
    for (int i = 0; i < NUM_LEVELS; i++)
        append_item(new level_menu_item(parent, i));

    append_action_item(SOUND_MENU_BACK, "back-sm-0.png", "back-sm-1.png")
        .set_on_clicked([parent] { parent->show_background(); })
        .set_action([parent] { parent->show_game_mode_menu(); })
        .set_is_back(true);
}

g2d::vec2 level_selection_menu::get_item_position(int item_index) const
{
    const float y_origin = .5 * (window_height + NUM_BUTTON_ROWS * ITEM_HEIGHT);

    const float offset_scale = 1. - get_cur_alpha();
    const float x_offset = (.5 * window_width + item_index * 96.) * offset_scale * offset_scale;

    if (item_index < NUM_LEVELS) {
        const float x_origin = .5 * (window_width - NUM_BUTTON_COLS * ITEM_WIDTH);

        const int r = item_index / NUM_BUTTON_COLS;
        const int c = item_index % NUM_BUTTON_COLS;

        return {x_origin + c * ITEM_WIDTH + x_offset, y_origin - (r + 1) * ITEM_HEIGHT};
    } else {
        const float x_origin = window_width - BACK_BUTTON_WIDTH;

        return {x_origin + x_offset, y_origin - NUM_BUTTON_ROWS * ITEM_HEIGHT - 2 * BACK_BUTTON_HEIGHT};
    }
}

void main_menu_impl::create_main_menu()
{
    main_menu_.append_action_item(SOUND_MENU_VALIDATE, "start-0.png", "start-1.png").set_action([this] {
        set_cur_menu(&game_mode_menu_);
    });

    main_menu_.append_action_item(SOUND_MENU_SELECT, "options-0.png", "options-1.png").set_action([this] {
        set_cur_menu(&options_menu_);
    });

    main_menu_.append_action_item(SOUND_MENU_SELECT, "more-0.png", "more-1.png").set_action([this] {
        set_cur_menu(&more_menu_);
    });

    main_menu_.append_action_item(SOUND_MENU_BACK, "quit-0.png", "quit-1.png")
        .set_on_clicked([this] { cur_state_ = state::OUTRO; })
        .set_action(quit)
        .set_is_back(true);
}

void main_menu_impl::create_more_menu()
{
    more_menu_.append_action_item(SOUND_MENU_SELECT, "stats-0.png", "stats-1.png")
        .set_on_clicked([this] { hide_background(); })
        .set_action(start_stats_page);

    more_menu_.append_action_item(SOUND_MENU_SELECT, "credits-0.png", "credits-1.png")
        .set_on_clicked([this] {
            background_.hide_billboard();
            start_sound(SOUND_LEVEL_INTRO, false);
        })
        .set_action(start_credits);

    more_menu_.append_action_item(SOUND_MENU_SELECT, "rate-me-0.png", "rate-me-1.png")
        .set_action(on_rate_me_clicked)
        .set_fade_menu_when_selected(false);

    more_menu_.append_action_item(SOUND_MENU_BACK, "back-0.png", "back-1.png")
        .set_action([this] { show_root_menu(); })
        .set_is_back(true);
}

void main_menu_impl::create_options_menu()
{
    options_menu_.append_toggle_item(SOUND_MENU_SELECT, cur_options->enable_hints, "hints-on-0.png", "hints-on-1.png",
                                     "hints-off-0.png", "hints-off-1.png");

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
        .set_action([this] { show_root_menu(); })
        .set_is_back(true);
}

void main_menu_impl::create_game_mode_menu()
{
    game_mode_menu_.append_action_item(SOUND_MENU_VALIDATE, "challenge-0.png", "challenge-1.png")
        .set_on_clicked([this] { hide_background(); })
        .set_action([this] {
            practice_mode = false;
            set_cur_menu(&level_menu_);
        });

    game_mode_menu_.append_action_item(SOUND_MENU_VALIDATE, "practice-0.png", "practice-1.png")
        .set_on_clicked([this] { hide_background(); })
        .set_action([this] {
            practice_mode = true;
            set_cur_menu(&level_menu_);
        });

    game_mode_menu_.append_action_item(SOUND_MENU_VALIDATE, "tutorial-0.png", "tutorial-1.png")
        .set_on_clicked([this] { set_state_outro(); })
        .set_action(start_tutorial);

    game_mode_menu_.append_action_item(SOUND_MENU_SELECT, "top-scores-0.png", "top-scores-1.png")
        .set_on_clicked([this] { hide_background(); })
        .set_action([this] { start_hiscore_list(); });

    game_mode_menu_.append_action_item(SOUND_MENU_BACK, "back-0.png", "back-1.png")
        .set_action([this] { show_root_menu(); })
        .set_is_back(true);
}

void draw_fade_overlay(float alpha)
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    render::set_color({1.f, 1.f, 1.f, alpha * alpha});
    render::draw_quad({{0, 0}, {0, window_height}, {window_width, window_height}, {window_width, 0}}, 10);
}

void main_menu_impl::reset()
{
    background_.reset();

#if 0
	if (cur_leaderboard->has_new_entry()) {
		cur_menu_ = &hiscore_menu;
		background_.fg_billboard->set_state(title_widget::OUTSIDE);
		background_.logo->set_state(title_widget::OUTSIDE);
	} else
#endif
    cur_menu_ = &main_menu_;

    cur_menu_->reset();

    fade_in();

    start_sound(SOUND_OPENING, false);
}

void main_menu_impl::redraw() const
{
    background_.draw();

    if (cur_state_ == state::FADE_IN) {
        draw_fade_overlay(1. - static_cast<float>(state_t_) / FADE_IN_T);
    } else if (cur_state_ == state::OUTRO) {
        draw_fade_overlay(1. - cur_menu_->get_cur_alpha());
    }

    cur_menu_->draw();
}

void main_menu_impl::update(uint32_t dt)
{
    state_t_ += dt;

    background_.update(dt);

    if (cur_state_ == state::FADE_IN) {
        if (state_t_ >= FADE_IN_T)
            cur_state_ = state::ACTIVE;
    } else {
        cur_menu_->update(dt);
    }
}

void main_menu_impl::on_touch_down(float x, float y)
{
    touch_is_down_ = true;
    cur_menu_->on_touch_down(x, y);
}

void main_menu_impl::on_touch_up()
{
    touch_is_down_ = false;
    cur_menu_->on_touch_up();
}

void main_menu_impl::on_touch_move(float x, float y)
{
    if (touch_is_down_)
        cur_menu_->on_touch_down(x, y);
}

void main_menu_impl::on_back_key()
{
    cur_menu_->on_back_key();
}

void main_menu_impl::on_menu_key()
{
}

main_menu_state::main_menu_state()
    : impl_(new main_menu_impl)
{
}

main_menu_state::~main_menu_state()
{
    delete impl_;
}

void main_menu_state::reset()
{
    impl_->reset();
}

void main_menu_state::redraw() const
{
    impl_->redraw();
}

void main_menu_state::update(uint32_t dt)
{
    impl_->update(dt);
}

void main_menu_state::on_touch_down(float x, float y)
{
    impl_->on_touch_down(x, y);
}

void main_menu_state::on_touch_up()
{
    impl_->on_touch_up();
}

void main_menu_state::on_touch_move(float x, float y)
{
    impl_->on_touch_move(x, y);
}

void main_menu_state::on_back_key()
{
    impl_->on_back_key();
}

void main_menu_state::on_menu_key()
{
    impl_->on_menu_key();
}

void main_menu_state::show_background()
{
    impl_->show_background();
}

void main_menu_state::hide_background()
{
    impl_->hide_background();
}

void main_menu_state::show_root_menu()
{
    impl_->show_root_menu();
}

void main_menu_state::show_more_menu()
{
    impl_->show_more_menu();
}

void main_menu_state::show_game_mode_menu()
{
    impl_->show_game_mode_menu();
}

void main_menu_state::fade_in()
{
    impl_->fade_in();
}

void main_menu_state::unfade()
{
    impl_->set_state_active();
}
