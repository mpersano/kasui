#include <cstdio>
#include <cassert>

#include <guava2d/vec2.h>

#include "render.h"
#include "common.h"
#include "sounds.h"
#include "menu.h"

menu_item::menu_item(int sound)
	: sound_(sound)
{
}

void
menu_item::reset()
{
	is_active_ = false;
}

void
menu_item::update(uint32_t dt)
{
	if (is_active_) {
		active_t_ += dt;
		if (active_t_ >= ACTIVE_T)
			is_active_ = false;
	}
}

float
menu_item::get_active_t() const
{
	return is_active_ ? static_cast<float>(active_t_)/ACTIVE_T : 0;
}

void
menu_item::on_activation()
{
	is_active_ = true;
	active_t_ = 0;
}

action_menu_item::action_menu_item(
		int sound,
		const std::string& active_sprite,
		const std::string& inactive_sprite,
		ActionFn on_activation_fn,
		ActionFn on_action_fn,
		bool is_back)
	: menu_item(sound)
	, active_sprite_(g2d::get_sprite(active_sprite))
	, inactive_sprite_(g2d::get_sprite(inactive_sprite))
	, on_activation_fn_(on_activation_fn)
	, on_action_fn_(on_action_fn)
	, is_back_(is_back)
{
}

void action_menu_item::draw(bool is_selected, float alpha) const
{
	render::set_color({ 1.f, 1.f, 1.f, is_enabled() ? alpha : alpha*.4f });
	(is_selected ? inactive_sprite_ : active_sprite_)->draw(0., 0., 50);
}

void action_menu_item::on_activation()
{
	menu_item::on_activation();
	if (on_activation_fn_)
		on_activation_fn_();
}

void action_menu_item::on_selection()
{
	if (on_action_fn_)
		on_action_fn_();
}

bool action_menu_item::fade_menu_when_selected() const
{
	return true;
}

bool action_menu_item::is_back_item() const
{
	return is_back_;
}

rect action_menu_item::get_rect() const
{
	return { static_cast<float>(active_sprite_->get_width()),
			 static_cast<float>(active_sprite_->get_height()) };
}

toggle_menu_item::toggle_menu_item(
		int sound,
		const std::string& active_sprite_true,
		const std::string& inactive_sprite_true,
		const std::string& active_sprite_false,
		const std::string& inactive_sprite_false,
		int& value_ptr,
		ToggleFn on_toggle_fn)
	: menu_item(sound)
	, active_sprite_true_(g2d::get_sprite(active_sprite_true))
	, inactive_sprite_true_(g2d::get_sprite(inactive_sprite_true))
	, active_sprite_false_(g2d::get_sprite(active_sprite_false))
	, inactive_sprite_false_(g2d::get_sprite(inactive_sprite_false))
	, value_ptr_(value_ptr)
	, on_toggle_fn_(on_toggle_fn)
{
}

void toggle_menu_item::draw(bool is_selected, float alpha) const
{
	render::set_color({ 1.f, 1.f, 1.f, is_enabled() ? alpha : alpha*.4f });

	const g2d::sprite *s;
	if (value_ptr_)
		s = is_selected ? inactive_sprite_true_ : active_sprite_true_;
	else
		s = is_selected ? inactive_sprite_false_ : active_sprite_false_;

	s->draw(0., 0., 50);
}

void toggle_menu_item::on_selection()
{
	value_ptr_ = !value_ptr_;
	if (on_toggle_fn_)
		on_toggle_fn_(value_ptr_);
}

bool toggle_menu_item::fade_menu_when_selected() const
{
	return false;
}

bool toggle_menu_item::is_back_item() const
{
	return false;
}

rect toggle_menu_item::get_rect() const
{
	return { static_cast<float>(active_sprite_true_->get_width()),
		 static_cast<float>(active_sprite_true_->get_height()) };
}

menu::menu()
: cur_selected_item_(nullptr)
{ }


void menu::append_action_item(
	int sound,
	const std::string& active_sprite,
	const std::string& inactive_sprite,
	action_menu_item::ActionFn on_activation_fn,
	action_menu_item::ActionFn on_action_fn,
	bool is_back)
{
	append_item(
		new action_menu_item(
			sound,
			active_sprite,
			inactive_sprite,
			on_activation_fn,
			on_action_fn,
			is_back));
}

void menu::append_toggle_item(
	int sound,
	const std::string& active_sprite_true,
	const std::string& inactive_sprite_true,
	const std::string& active_sprite_false,
	const std::string& inactive_sprite_false,
	int& value_ptr,
	toggle_menu_item::ToggleFn on_toggle_fn)
{
	append_item(
		new toggle_menu_item(
			sound,
			active_sprite_true, inactive_sprite_true,
			active_sprite_false, inactive_sprite_false,
			value_ptr,
			on_toggle_fn));
}

void
menu::append_item(menu_item *item)
{
	item_list_.emplace_back(item);
}

void
menu::draw() const
{
	render::set_blend_mode(blend_mode::ALPHA_BLEND);

	const float alpha = get_cur_alpha();

	for (size_t i = 0; i < item_list_.size(); ++i) {
		const g2d::vec2 pos = get_item_position(i);

		const auto p = item_list_[i].get();

		render::push_matrix();
		render::translate(pos.x, pos.y);
		p->draw(p == cur_selected_item_, alpha);
		render::pop_matrix();
	}
}

void
menu::update(uint32_t dt)
{
	state_t_ += dt;

	for (auto& p : item_list_)
		p->update(dt);

	switch (cur_state_) {
		case MENU_INTRO:
			if (state_t_ >= INTRO_T)
				set_cur_state(MENU_IDLE);
			break;

		case MENU_OUTRO:
			if (state_t_ >= OUTRO_T) {
				if (cur_selected_item_)
					cur_selected_item_->on_selection();
				set_cur_state(MENU_INACTIVE);
			}
			break;

		case MENU_IDLE:
		case MENU_INACTIVE:
			break;

		default:
			assert(0);
	}
}

void
menu::set_cur_state(state next_state)
{
	cur_state_ = next_state;
	state_t_ = 0;
}

void
menu::reset()
{
	set_cur_state(MENU_INTRO);

	cur_selected_item_ = nullptr;

	for (auto& p : item_list_)
		p->reset();
}

void
menu::on_touch_down(float x, float y)
{
	if (cur_state_ != MENU_IDLE)
		return;

	cur_selected_item_ = nullptr;

	int index = 0;

	for (auto& p : item_list_) {
		if (p->is_enabled()) {
			g2d::vec2 pos = get_item_position(index);

			const rect rc = p->get_rect();

			if (x >= pos.x && x < pos.x + rc.width &&
			    y >= pos.y && y < pos.y + rc.height) {
				cur_selected_item_ = p.get();
				break;
			}
		}

		++index;
	}
}

void
menu::on_touch_up()
{
	if (cur_state_ == MENU_IDLE && cur_selected_item_)
		activate_selected_item();
}

void
menu::on_back_key()
{
	if (cur_state_ == MENU_IDLE) {
		for (auto& p : item_list_) {
			if (p->is_back_item()) {
				cur_selected_item_ = p.get();
				activate_selected_item();
			}
		}
	}
}

void
menu::activate_selected_item()
{
	start_sound(cur_selected_item_->get_sound(), false);

	cur_selected_item_->on_activation();

	if (cur_selected_item_->fade_menu_when_selected())
		set_cur_state(MENU_OUTRO);
	else
		cur_selected_item_->on_selection();
}

float
menu::get_cur_alpha() const
{
	float alpha;

	switch (cur_state_) {
		case MENU_INTRO:
			alpha = static_cast<float>(state_t_)/INTRO_T;
			break;

		case MENU_OUTRO:
			alpha = 1. - static_cast<float>(state_t_)/OUTRO_T;
			break;

		case MENU_IDLE:
			alpha = 1.;
			break;

		case MENU_INACTIVE:
			alpha = 0;
			break;

		default:
			assert(0);
	}

	return alpha;
}

void
menu_initialize()
{ }
