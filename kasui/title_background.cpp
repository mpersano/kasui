#include "title_background.h"

#include "action.h"
#include "common.h"
#include "render.h"
#include "sprite_manager.h"
#include "tween.h"

#include <guava2d/texture_manager.h>

class title_background::widget
{
public:
    virtual ~widget() = default;

    virtual void update(uint32_t dt);
    virtual void reset();
    virtual void draw() const = 0;

    enum class state
    {
        OUTSIDE,
        ENTERING,
        INSIDE,
        LEAVING
    };

    void set_state(state next_state);
    void show();
    void hide();

protected:
    static constexpr int ENTER_T = 30 * MS_PER_TIC;
    static constexpr int LEAVE_T = 20 * MS_PER_TIC;

    state cur_state_ = state::INSIDE;
    uint32_t state_t_ = 0;
};

void title_background::widget::reset()
{
    set_state(state::ENTERING);
}

void title_background::widget::show()
{
    if (cur_state_ != state::INSIDE && cur_state_ != state::ENTERING)
        set_state(state::ENTERING);
}

void title_background::widget::hide()
{
    if (cur_state_ != state::OUTSIDE && cur_state_ != state::LEAVING)
        set_state(state::LEAVING);
}

void title_background::widget::set_state(state next_state)
{
    cur_state_ = next_state;
    state_t_ = 0;
}

void title_background::widget::update(uint32_t dt)
{
    state_t_ += dt;

    switch (cur_state_) {
        case state::ENTERING:
            if (state_t_ >= ENTER_T)
                set_state(state::INSIDE);
            break;

        case state::LEAVING:
            if (state_t_ >= LEAVE_T)
                set_state(state::OUTSIDE);
            break;

        default:
            break;
    }
}

namespace {

class kasui_logo : public title_background::widget
{
public:
    kasui_logo();

    void reset() override;
    void update(uint32_t dt) override;
    void draw() const override;

private:
    const g2d::sprite *bg_, *ka_, *sui_;
    float ka_scale_, sui_scale_;
    float ka_mix_, sui_mix_;
    std::unique_ptr<abstract_action> action_;
};

kasui_logo::kasui_logo()
    : bg_(g2d::get_sprite("logo-bg.png"))
    , ka_(g2d::get_sprite("logo-ka.png"))
    , sui_(g2d::get_sprite("logo-sui.png"))
    , ka_scale_(1)
    , sui_scale_(1)
    , ka_mix_(0)
    , sui_mix_(0)
{
    constexpr float max_scale = 1.15;

    action_.reset(
        (new parallel_action_group)
            ->add((new sequential_action_group)
                      ->add(new property_change_action<in_back_tween<float>>(&ka_scale_, 1, max_scale, 12 * MS_PER_TIC))
                      ->add(new property_change_action<out_bounce_tween<float>>(&ka_scale_, max_scale, 1,
                                                                                12 * MS_PER_TIC))
                      ->add(new delay_action(80 * MS_PER_TIC)))
            ->add(new property_change_action<linear_tween<float>>(&ka_mix_, 1, 0, 20 * MS_PER_TIC))
            ->add(
                (new sequential_action_group)
                    ->add(new delay_action(30 * MS_PER_TIC))
                    ->add(new property_change_action<in_back_tween<float>>(&sui_scale_, 1, max_scale, 12 * MS_PER_TIC))
                    ->add(new property_change_action<out_bounce_tween<float>>(&sui_scale_, max_scale, 1,
                                                                              12 * MS_PER_TIC)))
            ->add((new sequential_action_group)
                      ->add(new delay_action(30 * MS_PER_TIC))
                      ->add(new property_change_action<linear_tween<float>>(&sui_mix_, 1, 0, 20 * MS_PER_TIC))));
}

void kasui_logo::reset()
{
    widget::reset();

    ka_scale_ = sui_scale_ = 1;
    ka_mix_ = sui_mix_ = 0;
    action_->reset();
}

void kasui_logo::update(uint32_t dt)
{
    widget::update(dt);

    action_->step(dt);

    if (action_->done())
        action_->reset();
}

void kasui_logo::draw() const
{
    if (cur_state_ == state::OUTSIDE)
        return;

    constexpr float logo_y_from = 1., logo_y_to = 0.;
    float sy, alpha;

    switch (cur_state_) {
        case state::ENTERING: {
            const float t = static_cast<float>(state_t_) / ENTER_T;
            sy = out_bounce_tween<float>()(logo_y_from, logo_y_to, t);
            alpha = t;
            break;
        }

        case state::LEAVING: {
            const float t = static_cast<float>(state_t_) / LEAVE_T;
            sy = in_back_tween<float>()(logo_y_to, logo_y_from, t);
            alpha = 1. - t;
            break;
        }

        case state::INSIDE:
            sy = logo_y_to;
            alpha = 1;
            break;
    }

    const float w = bg_->get_width();
    const float h = bg_->get_height();

    const float x = .5 * (window_width - w);
    const float y = window_height - h + sy * h;

    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    render::set_color({1.f, 1.f, 1.f, alpha});

    render::push_matrix();
    render::translate(x, y);

    // background

    bg_->draw(0, 0, -15);

    // ka

    render::push_matrix();
    render::scale(1, ka_scale_);
    ka_->draw(0, 0, -10);
    render::pop_matrix();

    // sui

    render::scale(1, sui_scale_);
    sui_->draw(0, 0, -10);

    render::pop_matrix();
}

class foreground_billboard : public title_background::widget
{
public:
    foreground_billboard();

    void draw() const override;

private:
    const g2d::sprite *bb_;
    float y_;
    float scale_;
};

foreground_billboard::foreground_billboard()
    : bb_(g2d::get_sprite("haru-wo-bg.png"))
{
    const int w = bb_->get_width();
    const int h = bb_->get_height();

    float scaled_height = window_width * h / w;

    y_ = .5 * (window_height - scaled_height);
    if (y_ > 0)
        y_ = 0;

    scale_ = static_cast<float>(h) / scaled_height;
}

void foreground_billboard::draw() const
{
    if (cur_state_ == state::OUTSIDE)
        return;

    static const float x_from = -window_width, x_to = 0;
    float x, alpha;

    switch (cur_state_) {
        case state::ENTERING: {
            const float t = static_cast<float>(state_t_) / ENTER_T;
            x = in_cos_tween<float>()(x_from, x_to, t);
            alpha = t;
            break;
        }

        case state::LEAVING: {
            const float t = static_cast<float>(state_t_) / LEAVE_T;
            x = in_back_tween<float>()(x_to, x_from, t);
            alpha = 1. - t;
            break;
        }

        case state::INSIDE:
            x = x_to;
            alpha = 1;
            break;
    }

    render::push_matrix();

    render::translate(x, y_);
    render::scale(scale_, scale_);
    render::set_color({1.f, 1.f, 1.f, alpha});
    bb_->draw(0, 0, -10);

    render::pop_matrix();
}

} // anonymous namespace

title_background::title_background()
    : billboard_(new foreground_billboard)
    , logo_(new kasui_logo)
    , bg_texture_(g2d::load_texture("images/haru-bg.png"))
{
}

title_background::~title_background() = default;

void title_background::reset()
{
    sakura_.reset();

    billboard_->reset();
    logo_->reset();
}

void title_background::update(uint32_t dt)
{
    sakura_.update(dt);
    billboard_->update(dt);
    logo_->update(dt);
}

void title_background::draw() const
{
    render::set_blend_mode(blend_mode::NO_BLEND);
    draw_background_quad();

    render::set_blend_mode(blend_mode::ALPHA_BLEND);
    billboard_->draw();
    logo_->draw();
    sakura_.draw();
}

void title_background::draw_background_quad() const
{
    render::set_color({1.f, 1.f, 1.f, 1.f});

    const int w = bg_texture_->get_pixmap_width();
    const int h = bg_texture_->get_pixmap_height();

    const float scaled_height = std::max(window_width * h / w, window_height);

    const float dv = bg_texture_->get_v_scale();
    const float du = bg_texture_->get_u_scale();

    const float y1 = .5 * (window_height - scaled_height);
    const float y0 = y1 + scaled_height;

    render::draw_box(bg_texture_, {{0, y0}, {window_width, y1}}, {{0, 0}, {du, dv}}, -20);
}

void title_background::show_billboard()
{
    billboard_->show();
}

void title_background::hide_billboard()
{
    billboard_->hide();
}

void title_background::show_logo()
{
    logo_->show();
}

void title_background::hide_logo()
{
    logo_->hide();
}
