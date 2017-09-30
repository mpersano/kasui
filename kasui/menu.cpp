#include <cstdio>
#include <cassert>

#include "guava2d/g2dgl.h"
#include "guava2d/vec2.h"
#include "guava2d/texture.h"

#include "render.h"

#include "program_registry.h"
#include "common.h"
#include "sounds.h"
#include "menu.h"

void
menu_item::reset()
{
	is_active = false;
}

void
menu_item::update(uint32_t dt)
{
	if (is_active && (active_t += dt) >= ACTIVE_T)
		is_active = false;
}

float
menu_item::get_active_t() const
{
	return is_active ? static_cast<float>(active_t)/ACTIVE_T : 0;
}

void
menu_item::on_activation()
{
	is_active = true;
	active_t = 0;
}

menu::~menu()
{
}

menu::menu()
: cur_selected_item(0)
{ }

void
menu::append_item(menu_item *item)
{
	item_list.emplace_back(item);
}

void
menu::draw() const
{
	render::set_blend_mode(blend_mode::ALPHA_BLEND);

	const float alpha = get_cur_alpha();

	for (size_t i = 0; i < item_list.size(); ++i) {
		const g2d::vec2 pos = get_item_position(i);

		const auto p = item_list[i].get();

		render::push_matrix();
		render::translate(pos.x, pos.y);
		p->draw(p == cur_selected_item, alpha);
		render::pop_matrix();
	}
}

void
menu::update(uint32_t dt)
{
	state_t += dt;

	for (auto& p : item_list)
		p->update(dt);

	switch (cur_state) {
		case MENU_INTRO:
			if (state_t >= INTRO_T)
				set_cur_state(MENU_IDLE);
			break;

		case MENU_OUTRO:
			if (state_t >= OUTRO_T) {
				if (cur_selected_item)
					cur_selected_item->on_selection();
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
	cur_state = next_state;
	state_t = 0;
}

void
menu::reset()
{
	set_cur_state(MENU_INTRO);

	cur_selected_item = 0;

	for (auto& p : item_list)
		p->reset();
}

void
menu::on_touch_down(float x, float y)
{
	if (cur_state != MENU_IDLE)
		return;

	cur_selected_item = 0;

	int index = 0;

	for (auto& p : item_list) {
		if (p->is_enabled()) {
			g2d::vec2 pos = get_item_position(index);

			const rect rc = p->get_rect();

			if (x >= pos.x && x < pos.x + rc.width && y >= pos.y && y < pos.y + rc.height) {
				cur_selected_item = p.get();
				break;
			}
		}

		++index;
	}
}

void
menu::on_touch_up()
{
	if (cur_state == MENU_IDLE && cur_selected_item)
		activate_selected_item();
}

void
menu::on_back_key()
{
	if (cur_state == MENU_IDLE) {
		for (auto& p : item_list) {
			if (p->is_back_item()) {
				cur_selected_item = p.get();
				activate_selected_item();
			}
		}
	}
}

void
menu::activate_selected_item()
{
	start_sound(cur_selected_item->sound, false);

	cur_selected_item->on_activation();

	if (cur_selected_item->fade_menu_when_selected())
		set_cur_state(MENU_OUTRO);
	else
		cur_selected_item->on_selection();
}

float
menu::get_cur_alpha() const
{
	float alpha;

	switch (cur_state) {
		case MENU_INTRO:
			alpha = static_cast<float>(state_t)/INTRO_T;
			break;

		case MENU_OUTRO:
			alpha = 1 - static_cast<float>(state_t)/OUTRO_T;
			break;

		case MENU_IDLE:
			alpha = 1;
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
