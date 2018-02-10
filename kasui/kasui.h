#pragma once

class kasui_impl;
class http_request;

class kasui
{
public:
    static kasui &get_instance();

    void resize(int width, int height);

    void redraw();

    void on_pause();
    void on_resume();

    void on_touch_down(int x, int y);
    void on_touch_up();
    void on_touch_move(int x, int y);

    void on_back_key_pressed();
    void on_menu_key_pressed();

    void add_http_request(http_request *req);

private:
    kasui();
    ~kasui();

    kasui_impl *impl_;
};
