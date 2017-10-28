#include <algorithm>

#include <guava2d/vec2.h>

#include "render.h"
#include "common.h"
#include "sounds.h"
#include "menu.h"

menu_item::menu_item(int sound, bool fade_menu_when_selected)
	: sound_(sound)
	, fade_menu_when_selected_(fade_menu_when_selected)
{
}

void menu_item::draw(bool is_selected, float alpha) const
{
	render::set_color({ 1.f, 1.f, 1.f, is_enabled() ? alpha : alpha*.4f });
	get_sprite(is_selected)->draw(0., 0., 50);
}

void menu_item::reset()
{
	is_active_ = false;
}

void menu_item::update(uint32_t dt)
{
	if (is_active_) {
		active_t_ += dt;
		if (active_t_ >= ACTIVE_T)
			is_active_ = false;
	}
}

float menu_item::get_active_t() const
{
	return is_active_ ? static_cast<float>(active_t_)/ACTIVE_T : 0;
}

void menu_item::on_clicked()
{
	is_active_ = true;
	active_t_ = 0;
}

action_menu_item::action_menu_item(int sound, const std::string& active_sprite, const std::string& inactive_sprite)
	: menu_item(sound, true)
	, active_sprite_(g2d::get_sprite(active_sprite))
	, inactive_sprite_(g2d::get_sprite(inactive_sprite))
{
}

const g2d::sprite *action_menu_item::get_sprite(bool is_selected) const
{
	return is_selected ? inactive_sprite_ : active_sprite_;
}

void action_menu_item::on_clicked()
{
	if (on_clicked_)
		on_clicked_();
	menu_item::on_clicked();
}

void action_menu_item::action()
{
	if (action_)
		action_();
}

bool action_menu_item::is_back_item() const
{
	return is_back_;
}

rect action_menu_item::get_rect() const
{
	return { active_sprite_->get_width(), active_sprite_->get_height() };
}

toggle_menu_item::toggle_menu_item(
		int sound,
		int& value,
		const std::string& active_sprite_true,
		const std::string& inactive_sprite_true,
		const std::string& active_sprite_false,
		const std::string& inactive_sprite_false)
	: menu_item(sound, false)
	, value_(value)
	, active_sprite_true_(g2d::get_sprite(active_sprite_true))
	, inactive_sprite_true_(g2d::get_sprite(inactive_sprite_true))
	, active_sprite_false_(g2d::get_sprite(active_sprite_false))
	, inactive_sprite_false_(g2d::get_sprite(inactive_sprite_false))
{
}

const g2d::sprite *toggle_menu_item::get_sprite(bool is_selected) const
{
	if (value_)
		return is_selected ? inactive_sprite_true_ : active_sprite_true_;
	else
		return is_selected ? inactive_sprite_false_ : active_sprite_false_;
}

void toggle_menu_item::action()
{
	value_ = !value_;
	if (on_toggle_)
		on_toggle_(value_);
}

bool toggle_menu_item::is_back_item() const
{
	return false;
}

rect toggle_menu_item::get_rect() const
{
	return { active_sprite_true_->get_width(), active_sprite_true_->get_height() };
}

menu::menu() = default;

action_menu_item& menu::append_action_item(int sound, const std::string& active_sprite, const std::string& inactive_sprite)
{
	append_item(new action_menu_item(sound, active_sprite, inactive_sprite));
	return *static_cast<action_menu_item *>(item_list_.back().get());
}

toggle_menu_item& menu::append_toggle_item(int sound, int& value,
	const std::string& active_sprite_true, const std::string& inactive_sprite_true,
	const std::string& active_sprite_false, const std::string& inactive_sprite_false)
{
	append_item(new toggle_menu_item(sound, value,
				active_sprite_true, inactive_sprite_true,
				active_sprite_false, inactive_sprite_false));
	return *static_cast<toggle_menu_item *>(item_list_.back().get());
}

void menu::append_item(menu_item *item)
{
	item_list_.emplace_back(item);
}

void menu::draw() const
{
	render::set_blend_mode(blend_mode::ALPHA_BLEND);

	const float alpha = get_cur_alpha();

	for (size_t i = 0; i < item_list_.size(); ++i) {
		const auto p = item_list_[i].get();

		render::push_matrix();
		render::translate(get_item_position(i));
		p->draw(p == cur_selected_item_, alpha);
		render::pop_matrix();
	}
}

void menu::update(uint32_t dt)
{
	state_t_ += dt;

	for (auto& p : item_list_)
		p->update(dt);

	switch (cur_state_) {
		case state::INTRO:
			if (state_t_ >= INTRO_T)
				set_cur_state(state::IDLE);
			break;

		case state::OUTRO:
			if (state_t_ >= OUTRO_T) {
				if (cur_selected_item_)
					cur_selected_item_->action();
				set_cur_state(state::INACTIVE);
			}
			break;

		case state::IDLE:
		case state::INACTIVE:
			break;
	}
}

void menu::set_cur_state(state next_state)
{
	cur_state_ = next_state;
	state_t_ = 0;
}

void menu::reset()
{
	set_cur_state(state::INTRO);
	cur_selected_item_ = nullptr;

	for (auto& p : item_list_)
		p->reset();
}

void menu::on_touch_down(float x, float y)
{
	if (cur_state_ != state::IDLE)
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

void menu::on_touch_up()
{
	if (cur_state_ != state::IDLE)
		return;

	activate_selected_item();
}

void menu::on_back_key()
{
	if (cur_state_ != state::IDLE)
		return;

	auto it = std::find_if(std::begin(item_list_), std::end(item_list_),
			       [](auto& p) { return p->is_back_item(); });
	if (it != std::end(item_list_)) {
		cur_selected_item_ = it->get();
		activate_selected_item();
	}
}

void menu::activate_selected_item()
{
	if (!cur_selected_item_)
		return;

	start_sound(cur_selected_item_->get_sound(), false);

	cur_selected_item_->on_clicked();

	if (cur_selected_item_->fade_menu_when_selected())
		set_cur_state(state::OUTRO);
	else
		cur_selected_item_->action();
}

float menu::get_cur_alpha() const
{
	switch (cur_state_) {
		case state::INTRO:
			return static_cast<float>(state_t_)/INTRO_T;

		case state::OUTRO:
			return 1. - static_cast<float>(state_t_)/OUTRO_T;

		case state::IDLE:
			return 1.;

		case state::INACTIVE:
			return 0;
	}
}
