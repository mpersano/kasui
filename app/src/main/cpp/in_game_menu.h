#ifndef IN_GAME_MENU_H_
#define IN_GAME_MENU_H_

#include "state.h"

class in_game_menu_impl;

class in_game_menu_state : public state
{
public:
    in_game_menu_state();
    ~in_game_menu_state();

    void reset();
    void redraw() const;
    void update(uint32_t dt);
    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_touch_move(float x, float y);
    void on_back_key();
    void on_menu_key();

private:
    in_game_menu_impl *impl_;
};

#endif // IN_GAME_MENU_H_
