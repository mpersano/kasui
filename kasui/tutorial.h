#pragma once

#include "state.h"

class tutorial_state_impl;

class tutorial_state : public state
{
public:
	tutorial_state();
	~tutorial_state();

	void reset() override;
	void redraw() const override;
	void update(uint32_t dt) override;
	void on_touch_down(float x, float y) override;
	void on_touch_up() override;
	void on_touch_move(float x, float y) override;
	void on_back_key() override;
	void on_menu_key() override;

private:
	tutorial_state_impl *impl_;
};
