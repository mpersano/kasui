#pragma once

#include "state.h"

enum
{
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

    void reset() override;
    void redraw() const override;
    void update(uint32_t dt) override;
    void on_touch_down(float x, float y) override;
    void on_touch_up() override;
    void on_touch_move(float x, float y) override;
    void on_back_key() override;
    void on_menu_key() override;

    void set_enable_hints(bool enable);

private:
    in_game_state_impl *impl_;
};
