#ifndef MAIN_MENU_H_
#define MAIN_MENU_H_

#include "state.h"

class main_menu_impl;

class main_menu_state : public state
{
public:
	main_menu_state();
	~main_menu_state();

	void reset();
	void redraw() const;
	void update(uint32_t dt);
	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_touch_move(float x, float y);
	void on_back_key();
	void on_menu_key();

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

#endif // MAIN_MENU_H_
