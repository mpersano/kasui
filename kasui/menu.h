#pragma once

#include <string>
#include <vector>
#include <memory>

#include <guava2d/vec2.h>

#include "sprite_manager.h"
#include "sprite.h"
#include "render.h"

#include "common.h"

struct rect
{
	float width, height;
};

class menu_item
{
public:
	menu_item(int sound);
	virtual ~menu_item() = default;

	virtual void draw(bool is_selected, float alpha) const = 0;
	virtual void on_activation();
	virtual void on_selection() = 0;
	virtual bool fade_menu_when_selected() const = 0;
	virtual bool is_back_item() const = 0;
	virtual bool is_enabled() const { return true; }
	virtual rect get_rect() const = 0;

	void reset();
	void update(uint32_t dt);
	float get_active_t() const;

	int get_sound() const
	{ return sound_; }

private:
	static constexpr int ACTIVE_T = 10*MS_PER_TIC;

	int sound_;
	bool is_active_ = false;
	uint32_t active_t_;
};

class action_menu_item : public menu_item
{
public:
	using ActionFn = std::function<void()>;

	action_menu_item(
		int sound,
		const std::string& active_sprite,
		const std::string& inactive_sprite,
		ActionFn on_activation_fn,
		ActionFn on_action_fn,
		bool is_back);

	void draw(bool is_selected, float alpha) const override;
	void on_activation() override;
	void on_selection() override;
	bool fade_menu_when_selected() const override;
	bool is_back_item() const override;
	rect get_rect() const override;

private:
	const g2d::sprite *active_sprite_;
	const g2d::sprite *inactive_sprite_;
	ActionFn on_activation_fn_;
	ActionFn on_action_fn_;
	bool is_back_;
};

class toggle_menu_item : public menu_item
{
public:
	using ToggleFn = std::function<void(int)>;

	toggle_menu_item(
		int sound,
		const std::string& active_sprite_true,
		const std::string& inactive_sprite_true,
		const std::string& active_sprite_false,
		const std::string& inactive_sprite_false,
		int& value_ptr,
		ToggleFn on_toggle_fn);

	void draw(bool is_selected, float alpha) const override;
	void on_selection() override;
	bool fade_menu_when_selected() const;
	bool is_back_item() const override;
	rect get_rect() const override;

private:
	const g2d::sprite *active_sprite_true_, *inactive_sprite_true_;
	const g2d::sprite *active_sprite_false_, *inactive_sprite_false_;
	int& value_ptr_;
	ToggleFn on_toggle_fn_;
};

class menu
{
public:
	menu();
	virtual ~menu() = default;

	void append_action_item(
		int sound,
		const std::string& active_sprite,
		const std::string& inactive_sprite,
		action_menu_item::ActionFn on_activation_fn,
		action_menu_item::ActionFn on_action_fn,
		bool is_back = false);

	void append_toggle_item(
		int sound,
		const std::string& active_sprite_true,
		const std::string& inactive_sprite_true,
		const std::string& active_sprite_false,
		const std::string& inactive_sprite_false,
		int& value_ptr,
		toggle_menu_item::ToggleFn on_toggle_fn);

	void draw() const;
	void update(uint32_t dt);
	void reset();

	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_back_key();

	bool is_in_intro() const
	{ return cur_state_ == MENU_INTRO; }

	bool is_in_outro() const
	{ return cur_state_ == MENU_OUTRO; }

	float get_cur_alpha() const;

	void append_item(menu_item *item);

protected:
	void activate_selected_item();

	std::vector<std::unique_ptr<menu_item>> item_list_;
	uint32_t state_t_;
	menu_item *cur_selected_item_;

	static constexpr int INTRO_T = 15*MS_PER_TIC;
	static constexpr int OUTRO_T = 15*MS_PER_TIC;

	enum state {
		MENU_INTRO,
		MENU_IDLE,
		MENU_OUTRO,
		MENU_INACTIVE,
	} cur_state_;

	void set_cur_state(state next_state);

	virtual g2d::vec2 get_item_position(int item_index) const = 0;
};

void
menu_initialize();
