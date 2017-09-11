#ifndef IN_GAME_H_
#define IN_GAME_H_

#include "state.h"

enum {
	GRID_ROWS = 10,
	GRID_COLS = 6,
	BLOCK_SIZE = 42,
};

extern float grid_base_x, grid_base_y;

extern int total_tics;

class in_game_state_impl;

class in_game_state : public state
{
public:
	in_game_state();
	~in_game_state();

	void reset();
	void redraw() const;
	void update(uint32_t dt);
	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_touch_move(float x, float y);
	void on_back_key();
	void on_menu_key();

	void set_enable_hints(bool enable);

private:
	in_game_state_impl *impl_;
};

#endif // IN_GAME_H_
