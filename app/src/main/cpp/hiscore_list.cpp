#include "hiscore_list.h"
#include "common.h"
#include "leaderboard_page.h"
#include "main_menu.h"
#include "pause_button.h"
#include "tween.h"
#include "render.h"

class hiscore_list_impl
{
public:
    hiscore_list_impl();
    ~hiscore_list_impl();

    void reset();

    void redraw() const;
    void update(uint32_t dt);

    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_touch_move(float x, float y);

    void on_back_key();
    void on_menu_key();

private:
    leaderboard_page &leaderboard_;
    bool touch_is_down_;
    float last_touch_y_;
    pause_button pause_button_;

    static const int INTRO_TICS = 20 * MS_PER_TIC;
    static const int OUTRO_TICS = 20 * MS_PER_TIC;

    int state_tics_;
    enum state
    {
        INTRO,
        IDLE,
        OUTRO
    } state_;
};

hiscore_list_impl::hiscore_list_impl()
#ifdef FIX_ME
    : leaderboard_(get_net_leaderboard())
#else
    : leaderboard_(get_local_leaderboard())
#endif
    , pause_button_(window_width - pause_button::SIZE, window_height - pause_button::SIZE)
{
    pause_button_.set_callback([this] { on_back_key(); });
}

hiscore_list_impl::~hiscore_list_impl()
{
}

void hiscore_list_impl::reset()
{
    state_ = INTRO;
    state_tics_ = 0;
    touch_is_down_ = false;

    leaderboard_.reset();

    extern int total_tics;
    total_tics = 0;
}

void hiscore_list_impl::redraw() const
{
    get_prev_state()->redraw(); // draw main menu background

    float x_offset, alpha;

    switch (state_) {
        case INTRO: {
            float t = static_cast<float>(state_tics_) / INTRO_TICS;
            x_offset = out_bounce_tween<float>()(window_width, 0, t);
            alpha = t;
        } break;

        case OUTRO: {
            float t = 1. - static_cast<float>(state_tics_) / OUTRO_TICS;
            x_offset = in_back_tween<float>()(window_width, 0, t);
            alpha = t;
        } break;

        default:
            x_offset = 0;
            alpha = 1;
            break;
    }

    render::push_matrix();
    render::translate(x_offset, 0);
    leaderboard_.draw(alpha);
    render::pop_matrix();

    pause_button_.draw(alpha);
}

void hiscore_list_impl::update(uint32_t dt)
{
    extern int total_tics;
    total_tics += dt;

#if 1
    get_prev_state()->update(dt); // update main menu background
#endif
    leaderboard_.update(dt);

    switch (state_) {
        case INTRO:
            if ((state_tics_ += dt) >= INTRO_TICS)
                state_ = IDLE;
            break;

        case OUTRO:
            if ((state_tics_ += dt) >= OUTRO_TICS) {
                // lol
                main_menu_state *main_menu = static_cast<main_menu_state *>(get_prev_state());
                main_menu->show_game_mode_menu();

                // cur_leaderboard->reset_new_entry();

                pop_state();
            }
            break;

        default:
            break;
    }
}

void hiscore_list_impl::on_touch_down(float x, float y)
{
    if (pause_button_.on_touch_down(x, y))
        return;

    if (y < window_height - leaderboard_page::TITLE_HEIGHT) {
        touch_is_down_ = true;
        last_touch_y_ = y;
        leaderboard_.on_drag_start();
    }
}

void hiscore_list_impl::on_touch_up()
{
    if (pause_button_.on_touch_up())
        return;

    if (touch_is_down_) {
        leaderboard_.on_drag_end();
        touch_is_down_ = false;
    }
}

void hiscore_list_impl::on_touch_move(float x, float y)
{
    if (pause_button_.on_touch_down(x, y))
        return;

    if (touch_is_down_) {
        leaderboard_.on_drag(y - last_touch_y_);
        last_touch_y_ = y;
    }
}

void hiscore_list_impl::on_back_key()
{
    if (state_ == IDLE) {
        state_ = OUTRO;
        state_tics_ = 0;

        static_cast<main_menu_state *>(get_prev_state())->show_background();
    }
}

void hiscore_list_impl::on_menu_key()
{
}

hiscore_list_state::hiscore_list_state()
    : impl_(new hiscore_list_impl)
{
}

hiscore_list_state::~hiscore_list_state()
{
    delete impl_;
}

void hiscore_list_state::reset()
{
    impl_->reset();
}

void hiscore_list_state::redraw() const
{
    impl_->redraw();
}

void hiscore_list_state::update(uint32_t dt)
{
    impl_->update(dt);
}

void hiscore_list_state::on_touch_down(float x, float y)
{
    impl_->on_touch_down(x, y);
}

void hiscore_list_state::on_touch_up()
{
    impl_->on_touch_up();
}

void hiscore_list_state::on_touch_move(float x, float y)
{
    impl_->on_touch_move(x, y);
}

void hiscore_list_state::on_back_key()
{
    impl_->on_back_key();
}

void hiscore_list_state::on_menu_key()
{
    impl_->on_menu_key();
}
