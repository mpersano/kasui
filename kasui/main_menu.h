#pragma once

#include "state.h"

class main_menu_impl;

class main_menu_state : public state
{
public:
	main_menu_state();
	~main_menu_state();

	void reset() override;
	void redraw() const override;
	void update(uint32_t dt) override;
	void on_touch_down(float x, float y) override;
	void on_touch_up() override;
	void on_touch_move(float x, float y) override;
	void on_back_key() override;
	void on_menu_key() override;

	void show_background();
	void hide_background();
	void show_root_menu();
	void show_more_menu();
	void show_game_mode_menu();
	void fade_in();
	void unfade(); // GAH

private:
	main_menu_impl *impl_;
};
