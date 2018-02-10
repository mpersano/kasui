#pragma once

#include <memory>
#include <string>
#include <vector>

#include <guava2d/vec2.h>

#include "noncopyable.h"
#include "render.h"
#include "sprite.h"
#include "sprite_manager.h"

#include "common.h"

struct rect
{
    int width, height;
};

class menu_item
{
public:
    menu_item(int sound, bool fade_menu_when_selected);
    virtual ~menu_item() = default;

    virtual void on_clicked();
    virtual void action() = 0;
    virtual bool is_back_item() const = 0;
    virtual bool is_enabled() const { return true; }
    virtual rect get_rect() const = 0;
    virtual const g2d::sprite *get_sprite(bool is_selected) const = 0;

    void draw(bool is_selected, float alpha) const;
    void reset();
    void update(uint32_t dt);
    float get_active_t() const;

    int get_sound() const { return sound_; }

    bool fade_menu_when_selected() const { return fade_menu_when_selected_; }

    void set_fade_menu_when_selected(bool b) { fade_menu_when_selected_ = b; }

private:
    static constexpr int ACTIVE_T = 10 * MS_PER_TIC;

    int sound_;
    bool is_active_ = false;
    uint32_t active_t_;
    bool fade_menu_when_selected_;
};

class action_menu_item : public menu_item
{
public:
    using ActionCallback = std::function<void()>;

    action_menu_item(int sound, const std::string &active_sprite, const std::string &inactive_sprite);

    const g2d::sprite *get_sprite(bool is_selected) const override;
    void on_clicked() override;
    void action() override;
    bool is_back_item() const override;
    rect get_rect() const override;

    action_menu_item &set_on_clicked(ActionCallback f)
    {
        on_clicked_ = f;
        return *this;
    }

    action_menu_item &set_action(ActionCallback f)
    {
        action_ = f;
        return *this;
    }

    action_menu_item &set_is_back(bool b)
    {
        is_back_ = b;
        return *this;
    }

private:
    const g2d::sprite *active_sprite_;
    const g2d::sprite *inactive_sprite_;
    ActionCallback on_clicked_;
    ActionCallback action_;
    bool is_back_ = false;
};

class toggle_menu_item : public menu_item
{
public:
    using ToggleCallback = std::function<void(int)>;

    toggle_menu_item(int sound, int &value, const std::string &active_sprite_true,
                     const std::string &inactive_sprite_true, const std::string &active_sprite_false,
                     const std::string &inactive_sprite_false);

    const g2d::sprite *get_sprite(bool is_selected) const override;
    void action() override;
    bool is_back_item() const override;
    rect get_rect() const override;

    toggle_menu_item &set_on_toggle(ToggleCallback f)
    {
        on_toggle_ = f;
        return *this;
    }

private:
    int &value_;
    const g2d::sprite *active_sprite_true_, *inactive_sprite_true_;
    const g2d::sprite *active_sprite_false_, *inactive_sprite_false_;
    ToggleCallback on_toggle_;
};

class menu : private noncopyable
{
public:
    menu();
    virtual ~menu() = default;

    action_menu_item &append_action_item(int sound, const std::string &active_sprite,
                                         const std::string &inactive_sprite);

    toggle_menu_item &append_toggle_item(int sound, int &value, const std::string &active_sprite_true,
                                         const std::string &inactive_sprite_true,
                                         const std::string &active_sprite_false,
                                         const std::string &inactive_sprite_false);

    void append_item(menu_item *item);

    void draw() const;
    void update(uint32_t dt);
    void reset();

    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_back_key();

    bool is_in_intro() const { return cur_state_ == state::INTRO; }

    bool is_in_outro() const { return cur_state_ == state::OUTRO; }

    float get_cur_alpha() const;

private:
    void activate_selected_item();

    uint32_t state_t_ = 0;
    menu_item *cur_selected_item_ = nullptr;

    static constexpr int INTRO_T = 15 * MS_PER_TIC;
    static constexpr int OUTRO_T = 15 * MS_PER_TIC;

    enum class state
    {
        INTRO,
        IDLE,
        OUTRO,
        INACTIVE,
    } cur_state_ = state::INTRO;

    void set_cur_state(state next_state);

    virtual g2d::vec2 get_item_position(int item_index) const = 0;

protected:
    std::vector<std::unique_ptr<menu_item>> item_list_;
};
