#ifndef MENU_H_
#define MENU_H_

#include <vector>
#include <memory>

#include "guava2d/vec2.h"
#include "guava2d/vertex_array.h"
#include "guava2d/sprite.h"

#include "render.h"

#include "common.h"

namespace g2d {
class texture;
}

struct rect
{
	rect(float width, float height)
	: width(width), height(height)
	{ }

	float width, height;
};

struct menu_item
{
	menu_item(int sound)
	: sound(sound)
	, is_active(false)
	{ }

	virtual ~menu_item() { }

	virtual void draw(bool is_selected, float alpha) const = 0;

	virtual void on_activation();

	virtual void on_selection() = 0;

	virtual bool fade_menu_when_selected() const = 0;

	virtual bool is_back_item() const = 0;

	virtual bool is_enabled() const
	{
		return true;
	}

	virtual rect get_rect() const = 0;

	void reset();

	void update(uint32_t dt);

	float get_active_t() const;

	int sound;

	enum { ACTIVE_T = 10*MS_PER_TIC, };

	bool is_active;
	uint32_t active_t;
};

template <typename F, typename G>
struct action_menu_item : menu_item
{
	action_menu_item(
		int sound,
		const g2d::sprite *active_sprite,
		const g2d::sprite *inactive_sprite,
		F on_activation_fn,
		G on_action_fn,
		bool is_back)
	: menu_item(sound)
	, active_sprite(active_sprite)
	, inactive_sprite(inactive_sprite)
	, on_activation_fn(on_activation_fn)
	, on_action_fn(on_action_fn)
	, is_back(is_back)
	{ }

	void draw(bool is_selected, float alpha) const
	{
		render::set_color({ 1.f, 1.f, 1.f, is_enabled() ? alpha : alpha*.4f });
		render::draw_sprite(is_selected ? inactive_sprite : active_sprite, 0., 0., 50);
	}

	const g2d::sprite *get_sprite(bool active) const
	{
		return active ? active_sprite : inactive_sprite;
	}

	void on_activation()
	{
		menu_item::on_activation();
		on_activation_fn();
	}

	void on_selection()
	{
		on_action_fn();
	}

	bool fade_menu_when_selected() const
	{
		return true;
	}

	bool is_back_item() const
	{
		return is_back;
	}

	rect get_rect() const
	{
		return rect(active_sprite->get_width(), active_sprite->get_height());
	}

	const g2d::sprite *active_sprite;
	const g2d::sprite *inactive_sprite;
	F on_activation_fn;
	G on_action_fn;
	bool is_back;
};

template <typename T, typename F>
struct toggle_menu_item : menu_item
{
	toggle_menu_item(
		int sound,
		const g2d::sprite *active_sprite_true,
		const g2d::sprite *inactive_sprite_true,
		const g2d::sprite *active_sprite_false,
		const g2d::sprite *inactive_sprite_false,
		T *value_ptr,
		F on_toggle_fn)
	: menu_item(sound)
	, active_sprite_true(active_sprite_true)
	, inactive_sprite_true(inactive_sprite_true)
	, active_sprite_false(active_sprite_false)
	, inactive_sprite_false(inactive_sprite_false)
	, value_ptr(value_ptr)
	, on_toggle_fn(on_toggle_fn)
	{ }

	void draw(bool is_selected, float alpha) const
	{
		render::set_color({ 1.f, 1.f, 1.f, is_enabled() ? alpha : alpha*.4f });

		const g2d::sprite *s;
		if (*value_ptr)
			s = is_selected ? inactive_sprite_true : active_sprite_true;
		else
			s = is_selected ? inactive_sprite_false : active_sprite_false;

		render::draw_sprite(s, 0., 0., 50);
	}

	void on_selection()
	{
		*value_ptr = !*value_ptr;

		if (on_toggle_fn)
			on_toggle_fn(*value_ptr);
	}

	bool fade_menu_when_selected() const
	{ return false; }

	bool is_back_item() const
	{ return false; }

	rect get_rect() const
	{
		return rect(active_sprite_true->get_width(), active_sprite_true->get_height());
	}

	const g2d::sprite *active_sprite_true, *inactive_sprite_true;
	const g2d::sprite *active_sprite_false, *inactive_sprite_false;
	T *value_ptr;
	F on_toggle_fn;
};

class menu
{
public:
	menu();
	virtual ~menu();

	template <typename F, typename G>
	void append_action_item(
		int sound,
		const g2d::sprite *active_sprite,
		const g2d::sprite *inactive_sprite,
		F on_activation_fn,
		G on_action_fn,
		bool is_back = false)
	{
		append_item(
			new action_menu_item<F, G>(
				sound,
				active_sprite,
				inactive_sprite,
				on_activation_fn,
				on_action_fn,
				is_back));
	}

	template <typename T, typename F>
	void append_toggle_item(
		int sound,
		const g2d::sprite *active_sprite_true,
		const g2d::sprite *inactive_sprite_true,
		const g2d::sprite *active_sprite_false,
		const g2d::sprite *inactive_sprite_false,
		T *value_ptr,
		F on_toggle_fn)
	{
		append_item(
			new toggle_menu_item<T, F>(
				sound,
				active_sprite_true, inactive_sprite_true,
				active_sprite_false, inactive_sprite_false,
				value_ptr,
				on_toggle_fn));
	}

	virtual void draw() const;
	virtual void update(uint32_t dt);
	virtual void reset();

	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_back_key();

	bool is_in_intro() const
	{ return cur_state == MENU_INTRO; }

	bool is_in_outro() const
	{ return cur_state == MENU_OUTRO; }

	float get_cur_alpha() const;

	void append_item(menu_item *item);

protected:
	menu(const menu&); // disable copy ctor
	menu& operator=(const menu&); // disable assignment

	void activate_selected_item();

	std::vector<std::unique_ptr<menu_item>> item_list;
	uint32_t state_t;
	menu_item *cur_selected_item;

	enum {
		INTRO_T = 15*MS_PER_TIC,
		OUTRO_T = 15*MS_PER_TIC,
	};

	enum state {
		MENU_INTRO,
		MENU_IDLE,
		MENU_OUTRO,
		MENU_INACTIVE,
	};

	state cur_state;

	void set_cur_state(state next_state);

	virtual g2d::vec2 get_item_position(int item_index) const = 0;
};

void
menu_initialize();

#endif // MENU_H_
