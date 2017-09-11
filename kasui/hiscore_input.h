#pragma once

#include "state.h"

class hiscore_input_state_impl;

class hiscore_input_state : public state
{
public:
	hiscore_input_state();
	~hiscore_input_state();

	void reset();
	void redraw() const;
	void update(uint32_t dt);
	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_touch_move(float x, float y);
	void on_back_key();
	void on_menu_key();

	void set_score(int score);

private:
	hiscore_input_state_impl *impl_;
};
