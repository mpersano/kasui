#include <cstdlib>
#include <cstring>

#include <guava2d/panic.h>
#include <guava2d/file.h>
#include <guava2d/texture_manager.h>
#include <guava2d/xwchar.h>

#include "background.h"
#include "common.h"
#include "main_menu.h"
#include "pause_button.h"
#include "render.h"
#include "sprite.h"
#include "sprite_manager.h"
#include "text_box.h"
#include "theme.h"
#include "tutorial.h"
#include "tween.h"
#include "utf8.h"
#include "world.h"
#include "settings.h"

enum
{
    TEXT_FADE_IN_TICS = 30 * MS_PER_TIC,
    TEXT_FADE_OUT_TICS = 15 * MS_PER_TIC,
};

class tutorial_cmd;

class tutorial_state_impl
{
public:
    tutorial_state_impl();

    void reset();
    void redraw() const;
    void update(uint32_t dt);
    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_touch_move(float x, float y);
    void on_back_key();
    void on_menu_key();

    void set_falling_blocks(wchar_t left, wchar_t right);
    void set_grid_row(int row_index, const wchar_t *row_kanji);
    void set_text(const wchar_t *text);
    void hide_text();
    void start_finger_animation(int from_x, int from_y, int to_x, int to_y, int tics);

    void press_left();
    void press_right();
    void press_up();
    void press_down();

    void wait(int tics);
    void update_world(int tics);

private:
    void load_script(); // XXX: move this somewhere else later

    world world_;
    text_box text_box_;
    std::unique_ptr<theme_animation> theme_;
    const color_scheme *colors_;
    float grid_base_x_, grid_base_y_;

    bool show_text_box_;

    std::vector<tutorial_cmd *> script_;
    std::vector<tutorial_cmd *>::iterator cur_cmd_;

    enum state
    {
        STATE_TEXT_FADE_IN,
        STATE_TEXT_FADE_OUT,
        STATE_UPDATE_WORLD,
        STATE_WAIT,
        STATE_IDLE,
        STATE_FINGER_ANIMATION,
    } cur_state_;

    int state_tics_;

    struct finger_animation
    {
        int from_x, from_y;
        int to_x, to_y;
        int tics;
    } finger_animation_;

    const g2d::sprite *finger_;

    pause_button pause_button_;
};

class tutorial_cmd
{
public:
    tutorial_cmd(tutorial_state_impl &tutorial)
        : tutorial_(tutorial)
    {
    }

    virtual ~tutorial_cmd() {}

    virtual void execute() const = 0;

protected:
    tutorial_state_impl &tutorial_;
};

class set_grid_row_cmd : public tutorial_cmd
{
public:
    set_grid_row_cmd(tutorial_state_impl &tutorial, int row_index, const wchar_t *row_kanji)
        : tutorial_cmd(tutorial)
        , row_index_(row_index)
        , row_kanji_(row_kanji)
    {
    }

    void execute() const override { tutorial_.set_grid_row(row_index_, row_kanji_); }

private:
    int row_index_;
    const wchar_t *row_kanji_;
};

class set_falling_blocks_cmd : public tutorial_cmd
{
public:
    set_falling_blocks_cmd(tutorial_state_impl &tutorial, wchar_t left, wchar_t right)
        : tutorial_cmd(tutorial)
        , left_(left)
        , right_(right)
    {
    }

    void execute() const override { tutorial_.set_falling_blocks(left_, right_); }

private:
    wchar_t left_, right_;
};

class finger_animation_cmd : public tutorial_cmd
{
public:
    finger_animation_cmd(tutorial_state_impl &tutorial, int from_x, int from_y, int to_x, int to_y, int tics)
        : tutorial_cmd(tutorial)
        , from_x_(from_x)
        , from_y_(from_y)
        , to_x_(to_x)
        , to_y_(to_y)
        , tics_(tics)
    {
    }

    void execute() const override { tutorial_.start_finger_animation(from_x_, from_y_, to_x_, to_y_, tics_); }

private:
    int from_x_, from_y_;
    int to_x_, to_y_;
    int tics_;
};

class set_text_cmd : public tutorial_cmd
{
public:
    set_text_cmd(tutorial_state_impl &tutorial, const wchar_t *text)
        : tutorial_cmd(tutorial)
        , text_(text)
    {
    }

    void execute() const override { tutorial_.set_text(text_); }

private:
    const wchar_t *text_;
};

class hide_text_cmd : public tutorial_cmd
{
public:
    hide_text_cmd(tutorial_state_impl &tutorial)
        : tutorial_cmd(tutorial)
    {
    }

    void execute() const override { tutorial_.hide_text(); }
};

class wait_cmd : public tutorial_cmd
{
public:
    wait_cmd(tutorial_state_impl &tutorial, int tics)
        : tutorial_cmd(tutorial)
        , tics_(tics)
    {
    }

    void execute() const override { tutorial_.wait(tics_); }

private:
    int tics_;
};

class press_left_cmd : public tutorial_cmd
{
public:
    press_left_cmd(tutorial_state_impl &tutorial)
        : tutorial_cmd(tutorial)
    {
    }

    void execute() const override { tutorial_.press_left(); }
};

class press_right_cmd : public tutorial_cmd
{
public:
    press_right_cmd(tutorial_state_impl &tutorial)
        : tutorial_cmd(tutorial)
    {
    }

    void execute() const override { tutorial_.press_right(); }
};

class press_up_cmd : public tutorial_cmd
{
public:
    press_up_cmd(tutorial_state_impl &tutorial)
        : tutorial_cmd(tutorial)
    {
    }

    void execute() const override { tutorial_.press_up(); }
};

class press_down_cmd : public tutorial_cmd
{
public:
    press_down_cmd(tutorial_state_impl &tutorial)
        : tutorial_cmd(tutorial)
    {
    }

    void execute() const override { tutorial_.press_down(); }
};

class update_world_cmd : public tutorial_cmd
{
public:
    update_world_cmd(tutorial_state_impl &tutorial, int tics)
        : tutorial_cmd(tutorial)
        , tics_(tics)
    {
    }

    void execute() const override { tutorial_.update_world(tics_); }

private:
    int tics_;
};

tutorial_state_impl::tutorial_state_impl()
    : world_(8, 6, window_height - text_box::HEIGHT - 40)
    , text_box_(window_width)
    , finger_(g2d::get_sprite("finger.png"))
    , pause_button_(window_width - pause_button::SIZE, window_height - pause_button::SIZE)
{
    pause_button_.set_callback([this] { on_back_key(); });

    // world_.initialize_grid(2);

    grid_base_x_ = .5 * (window_width - world_.get_width());
    grid_base_y_ = .5 * (window_height - text_box::HEIGHT - world_.get_height());
    ;

    load_script();

    reset();
}

void tutorial_state_impl::load_script()
{
    g2d::file_input_stream reader("data/tutorial");

    static char line[1024];

    int lineno = 0;

    while (reader.gets(line, sizeof line)) {
        ++lineno;

        if (*line == '#')
            continue;

        int len = strlen(line);

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        switch (*line) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                script_.push_back(new set_grid_row_cmd(*this, *line - '0', utf8_to_wchar(line + 2)));
                break;

            case 'f': {
                wchar_t *arg = utf8_to_wchar(line + 2);
                script_.push_back(new set_falling_blocks_cmd(*this, arg[0], arg[1]));
                delete[] arg;
            } break;

            case 't':
                script_.push_back(new set_text_cmd(*this, utf8_to_wchar(line + 2)));
                break;

            case 'h':
                script_.push_back(new hide_text_cmd(*this));
                break;

            case 'l':
                script_.push_back(new press_left_cmd(*this));
                break;

            case 'r':
                script_.push_back(new press_right_cmd(*this));
                break;

            case 'u':
                script_.push_back(new press_up_cmd(*this));
                break;

            case 'd':
                script_.push_back(new press_down_cmd(*this));
                break;

            case 'w':
                script_.push_back(new wait_cmd(*this, atoi(line + 2)));
                break;

            case 'g':
                script_.push_back(new update_world_cmd(*this, atoi(line + 2)));
                break;

            case '^': {
                int from_x, from_y, to_x, to_y, tics;
                sscanf(line + 2, "%d %d %d %d %d", &from_x, &from_y, &to_x, &to_y, &tics);
                script_.push_back(new finger_animation_cmd(*this, from_x, from_y, to_x, to_y, tics));
            } break;

            default:
                panic("%s: invalid tutorial command in line %d: `%c'", __func__, lineno, *line);
                break;
        }
    }

    cur_cmd_ = script_.begin();
}

void tutorial_state_impl::reset()
{
    const auto theme_index = rand() % NUM_THEMES;

    theme_ = make_theme(theme_index);
    colors_ = &cur_settings.color_schemes[theme_index];

    theme_->reset();

    const auto& gradient = colors_->background_gradient;
    background_initialize_gradient(gradient.from, gradient.to);

    world_.set_theme_colors(colors_->main_color, colors_->alternate_color);
    world_.set_text_gradient(colors_->text_gradient);

    cur_cmd_ = script_.begin();

    world_.reset();
    world_.set_level(0, true, false);
    world_.initialize_grid(0);

    cur_state_ = STATE_IDLE;
    show_text_box_ = false;
}

void tutorial_state_impl::redraw() const
{
    background_draw_gradient();

    theme_->draw();

    render::push_matrix();
    render::translate(grid_base_x_, grid_base_y_);
    world_.draw();
    render::pop_matrix();

    if (show_text_box_) {
        float scale, alpha;

        switch (cur_state_) {
            case STATE_TEXT_FADE_IN: {
                const float t = 1. - static_cast<float>(state_tics_) / TEXT_FADE_IN_TICS;
                scale = out_bounce_tween<float>()(.5, 1, t);
                alpha = t;
            } break;

            case STATE_TEXT_FADE_OUT: {
                const float t = 1. - static_cast<float>(state_tics_) / TEXT_FADE_OUT_TICS;
                scale = in_back_tween<float>()(1, .5, t);
                alpha = 1. - t;
            } break;

            default:
                scale = alpha = 1;
                break;
        }

        render::push_matrix();
        render::translate(.5 * window_width, window_height - .5 * text_box::HEIGHT);
        render::scale(scale, scale);

        text_box_.draw(alpha);

        render::pop_matrix();
    }

    if (cur_state_ == STATE_FINGER_ANIMATION) {
        // alpha

        static const float FADE_TICS = 5 * MS_PER_TIC;

        float alpha;

        if (state_tics_ < FADE_TICS)
            alpha = static_cast<float>(state_tics_) / FADE_TICS;
        else if (state_tics_ > finger_animation_.tics - FADE_TICS)
            alpha = static_cast<float>(finger_animation_.tics - state_tics_) / FADE_TICS;
        else
            alpha = 1;

        // position

        float t = 1. - static_cast<float>(state_tics_) / finger_animation_.tics;

        float x = finger_animation_.from_x + t * (finger_animation_.to_x - finger_animation_.from_x) + grid_base_x_ +
                  .5 * world_.get_width() - .5 * finger_->get_width();

        float y = finger_animation_.from_y + t * (finger_animation_.to_y - finger_animation_.from_y) + grid_base_x_ +
                  .5 * world_.get_width() - .5 * finger_->get_width();

        render::set_blend_mode(blend_mode::ALPHA_BLEND);
        render::set_color({1.f, 1.f, 1.f, alpha});
        render::push_matrix();
        render::translate(x, y);
        finger_->draw(0, 0, 20);
        render::pop_matrix();
    }

    pause_button_.draw(1);
}

void tutorial_state_impl::update(uint32_t dt)
{
    theme_->update(dt);

#if 0
	if (dpad_state & DPAD_LEFT)
		world_.on_left_pressed();

	if (dpad_state & DPAD_RIGHT)
		world_.on_right_pressed();

	if (dpad_state & DPAD_UP)
		world_.on_up_pressed();

	if (dpad_state & DPAD_DOWN)
		world_.on_down_pressed();

	world_.update(dt);
#else

    switch (cur_state_) {
        case STATE_IDLE:
            while (cur_cmd_ != script_.end() && cur_state_ == STATE_IDLE)
                (*cur_cmd_++)->execute();
            break;

        case STATE_TEXT_FADE_IN:
        case STATE_WAIT:
        case STATE_FINGER_ANIMATION:
            if ((state_tics_ -= dt) <= 0)
                cur_state_ = STATE_IDLE;
            break;

        case STATE_TEXT_FADE_OUT:
            if ((state_tics_ -= dt) <= 0) {
                show_text_box_ = false;
                cur_state_ = STATE_IDLE;
            }
            break;

        case STATE_UPDATE_WORLD:
            world_.update(dt);
            if ((state_tics_ -= dt) <= 0)
                cur_state_ = STATE_IDLE;
            break;

        default:
            assert(0);
    }
#endif
}

void tutorial_state_impl::set_falling_blocks(wchar_t left, wchar_t right)
{
    world_.set_falling_blocks(left, right);
}

void tutorial_state_impl::set_grid_row(int row_index, const wchar_t *row_kanji)
{
    world_.set_row(row_index, row_kanji);
}

void tutorial_state_impl::set_text(const wchar_t *text)
{
    text_box_.set_text(text);

    cur_state_ = STATE_TEXT_FADE_IN;
    show_text_box_ = true;
    state_tics_ = TEXT_FADE_IN_TICS;
}

void tutorial_state_impl::hide_text()
{
    cur_state_ = STATE_TEXT_FADE_OUT;
    state_tics_ = TEXT_FADE_OUT_TICS;
}

void tutorial_state_impl::start_finger_animation(int from_x, int from_y, int to_x, int to_y, int tics)
{
    cur_state_ = STATE_FINGER_ANIMATION;

    finger_animation_.from_x = from_x;
    finger_animation_.from_y = from_y;

    finger_animation_.to_x = to_x;
    finger_animation_.to_y = to_y;

    finger_animation_.tics = state_tics_ = tics * MS_PER_TIC;
}

void tutorial_state_impl::press_left()
{
    world_.on_left_pressed();
}

void tutorial_state_impl::press_right()
{
    world_.on_right_pressed();
}

void tutorial_state_impl::press_up()
{
    world_.on_up_pressed();
}

void tutorial_state_impl::press_down()
{
    world_.on_down_pressed();
}

void tutorial_state_impl::wait(int tics)
{
    cur_state_ = STATE_WAIT;
    state_tics_ = tics * MS_PER_TIC;
}

void tutorial_state_impl::update_world(int tics)
{
    cur_state_ = STATE_UPDATE_WORLD;
    state_tics_ = tics * MS_PER_TIC;
}

void tutorial_state_impl::on_touch_down(float x, float y)
{
    if (!pause_button_.on_touch_down(x, y)) {
        if (cur_cmd_ == script_.end())
            on_back_key();
    }
}

void tutorial_state_impl::on_touch_up()
{
    pause_button_.on_touch_up();
}

void tutorial_state_impl::on_touch_move(float x, float y)
{
    pause_button_.on_touch_down(x, y);
}

void tutorial_state_impl::on_back_key()
{
    main_menu_state *main_menu = static_cast<main_menu_state *>(get_prev_state());
    main_menu->show_background();
    main_menu->show_game_mode_menu();
    main_menu->fade_in();

    pop_state();
}

void tutorial_state_impl::on_menu_key()
{
}

tutorial_state::tutorial_state()
    : impl_(new tutorial_state_impl)
{
}

tutorial_state::~tutorial_state()
{
    delete impl_;
}

void tutorial_state::reset()
{
    impl_->reset();
}

void tutorial_state::redraw() const
{
    impl_->redraw();
}

void tutorial_state::update(uint32_t dt)
{
    impl_->update(dt);
}

void tutorial_state::on_touch_down(float x, float y)
{
    impl_->on_touch_down(x, y);
}

void tutorial_state::on_touch_up()
{
    impl_->on_touch_up();
}

void tutorial_state::on_touch_move(float x, float y)
{
    impl_->on_touch_move(x, y);
}

void tutorial_state::on_back_key()
{
    impl_->on_back_key();
}

void tutorial_state::on_menu_key()
{
    impl_->on_menu_key();
}
