#ifndef STATS_PAGE_H_
#define STATS_PAGE_H_

#include "state.h"

class stats_state_impl;

class stats_page_state : public state
{
public:
	stats_page_state();
	~stats_page_state();

	void reset();
	void redraw() const;
	void update(uint32_t dt);
	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_touch_move(float x, float y);
	void on_back_key();
	void on_menu_key();

private:
	stats_state_impl *impl_;
};

#endif // STATS_H_
