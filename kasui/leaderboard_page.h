#pragma once

#include <string>
#include <vector>

#include <guava2d/draw_queue.h>

namespace g2d {
class mat4;
};

class item;

class leaderboard_event_listener
{
public:
    virtual ~leaderboard_event_listener() {}

    virtual void on_check_hiscore_response(bool ok, bool is_hiscore) = 0;
};

class leaderboard_page
{
public:
    leaderboard_page(const std::string &title, const std::string &cache_path);
    virtual ~leaderboard_page();

    leaderboard_page(const leaderboard_page &) = delete;
    leaderboard_page &operator=(const leaderboard_page &) = delete;

    virtual void reset();

    virtual void draw(const g2d::mat4 &mat, float alpha) const = 0;

    virtual bool async_check_hiscore(leaderboard_event_listener *listener, int score) = 0;
    virtual bool async_insert_hiscore(leaderboard_event_listener *listener, const wchar_t *name, int score) = 0;

    void update(uint32_t dt);

    void on_drag_start();
    void on_drag_end();
    void on_drag(float dy);

    static const int TITLE_HEIGHT = 96;

protected:
    static const int MAX_HISCORES = 40;

    bool is_hiscore(int score) const;
    bool load_cache();
    void save_cache() const;
    void draw_title(const g2d::mat4 &mat, float alpha) const;
    void draw_items(const g2d::mat4 &mat, float alpha) const;

    void update_y_offset(float dy);
    void clear_items();

    std::string cache_path_;

    g2d::draw_queue title_text_;
    std::vector<item *> items_;
    float y_offset_;
    int touch_start_tic_;
    float total_drag_dy_;
    float speed_;
    int tics_;
};

leaderboard_page &get_net_leaderboard();

leaderboard_page &get_local_leaderboard();
