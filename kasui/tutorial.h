#pragma once

#include "state.h"

class tutorial_state_impl;

class tutorial_state : public state
{
public:
	tutorial_state();
	~tutorial_state();

	void reset();
	void redraw() const;
	void update(uint32_t dt);
	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_touch_move(float x, float y);
	void on_back_key();
	void on_menu_key();

private:
	tutorial_state_impl *impl_;
};
