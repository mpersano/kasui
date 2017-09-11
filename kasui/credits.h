#ifndef CREDITS_H_
#define CREDITS_H_


#include "state.h"

class credits_impl;

class credits_state : public state
{
public:
	credits_state();
	~credits_state();

	void reset();
	void redraw() const;
	void update(uint32_t dt);
	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_touch_move(float x, float y);
	void on_back_key();
	void on_menu_key();

private:
	credits_impl *impl_;
};

#endif // CREDITS_H_
