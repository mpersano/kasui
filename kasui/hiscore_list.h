#ifndef HISCORE_LIST_H_
#define HISCORE_LIST_H_

#include "state.h"

class hiscore_list_impl;

class hiscore_list_state : public state
{
public:
    hiscore_list_state();
    ~hiscore_list_state();

    void reset();
    void redraw() const;
    void update(uint32_t dt);
    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_touch_move(float x, float y);
    void on_back_key();
    void on_menu_key();

private:
    hiscore_list_impl *impl_;
};

#endif // HISCORE_LIST_H_
