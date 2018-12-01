#include "leaderboard_page.h"

#include "utils.h"
#include "render.h"
#include "common.h"
#include "http_request.h"
#include "kasui.h"
#include "log.h"
#include "utf8.h"
#include "fonts.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <sys/stat.h>
#include <time.h>

#include <algorithm>
#include <list>

#include <iomanip>
#include <sstream>
#include <cassert>

#include <guava2d/rgb.h>
#include <guava2d/texture_manager.h>
#include <guava2d/xwchar.h>

void draw_message(const g2d::mat4 &mat, float alpha, const wchar_t *text)
{
#ifdef FIX_ME
    const auto *small_font = get_font(font::tiny);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto &prog = get_program_instance<program_timer_text>();
    prog.use();
    prog.set_proj_modelview_matrix(mat);
    prog.set_texture(0);

    prog.set_text_color(g2d::rgba(1., 1., 1., alpha));
    prog.set_outline_color(g2d::rgba(.5, 0., 0., alpha));

    g2d::draw_queue()
        .text_program(prog.get_raw())
        .translate(.5 * window_width, .5 * window_height)
        .scale(1.2)
        .align_center()
        .render_text(small_font, L"%s", text)
        .draw();
#endif
}

class item
{
public:
    item(int rank, const std::wstring &name, int score, bool highlight);

    void draw(float alpha) const;

    int get_rank() const { return rank_; }
    int get_score() const { return score_; }
    const std::wstring &get_name() const { return name_; }

    void set_rank(int rank)
    {
        rank_ = rank;
        initialize_text();
    }

    bool get_highlight() const { return highlight_; }

    static const int HEIGHT = 80;

private:
    void initialize_text();

    static const int BORDER_RADIUS = 16;

    int rank_;
    std::wstring name_;
    int score_;
    bool highlight_;
    const g2d::texture *frame_texture_;
    std::wstring rank_text_;
    std::wstring score_text_;
};

item::item(int rank, const std::wstring &name, int score, bool highlight)
    : rank_(rank)
    , name_(name)
    , score_(score)
    , highlight_(highlight)
    , frame_texture_(g2d::load_texture("images/w-button-border.png"))
{
    initialize_text();
}

void item::initialize_text()
{
    rank_text_ = format_number(rank_);
    score_text_ = format_number(score_);
}

void item::draw(float alpha) const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    // frame

    {
        constexpr const auto FRAME_LAYER = 100;

        g2d::rgba color;

        if (!highlight_) {
            color = g2d::rgba(1, 1, 1, alpha);
        } else {
            extern int total_tics;
            float t = .875 + .125 * sin(.2 * total_tics);
            color = g2d::rgba(1, t, t, alpha);
        }

        const float x0 = 0;
        const float x1 = BORDER_RADIUS;

        const float x3 = window_width;
        const float x2 = x3 - BORDER_RADIUS;

        const float y0 = 0;
        const float y1 = -BORDER_RADIUS;

        const float y3 = -HEIGHT;
        const float y2 = y3 + BORDER_RADIUS;

        const auto draw_row = [this, x0, x1, x2, x3](float y0, float y1, float u0, float u1) {
            render::draw_quad(
                    frame_texture_,
                    {{x0, y0}, {x0, y1}, {x1, y1}, {x1, y0}},
                    {{0, u0}, {0, u1}, {.25, u1}, {.25, u0}},
                    FRAME_LAYER);

            render::draw_quad(
                    frame_texture_,
                    {{x1, y0}, {x1, y1}, {x2, y1}, {x2, y0}},
                    {{.25, u0}, {.25, u1}, {.75, u1}, {.75, u0}},
                    FRAME_LAYER);

            render::draw_quad(
                    frame_texture_,
                    {{x2, y0}, {x2, y1}, {x3, y1}, {x3, y0}},
                    {{.75, u0}, {.75, u1}, {1, u1}, {1, u0}},
                    FRAME_LAYER);
        };

        draw_row(y0, y1, 0, .25);
        draw_row(y1, y2, .25, .75);
        draw_row(y2, y3, .75, 1);
    }

    constexpr const auto TEXT_LAYER = 101;

    // score

    {
        render::push_matrix();

        render::translate(window_width - 28, -52.5);

        const auto top_color = g2d::rgb{200, 200, 255} * (1. / 255.);
        const auto bottom_color = g2d::rgb{120, 120, 255} * (1. / 255.);

        const g2d::rgba top_color_text{top_color, alpha};
        const g2d::rgba bottom_color_text{bottom_color, alpha};
        const g2d::rgba top_color_outline{.5 * top_color, alpha};
        const g2d::rgba bottom_color_outline{.5 * bottom_color, alpha};

        render::set_text_align(text_align::RIGHT);
        render::draw_text(get_font(font::small), {}, TEXT_LAYER,
                top_color_outline, top_color_text,
                bottom_color_outline, bottom_color_text,
                score_text_.c_str());

        render::pop_matrix();
    }

    // rank and name

    render::set_blend_mode(blend_mode::INVERSE_BLEND);

    {
        const auto *small_font = get_font(font::small);
        const auto *tiny_font = get_font(font::tiny);

        auto gi_small = small_font->find_glyph(L'X');
        const float y_small = .5 * (-HEIGHT + gi_small->height) - gi_small->top;

        auto gi_tiny = tiny_font->find_glyph(L'X');
        const float y_tiny = .5 * (-HEIGHT + gi_tiny->height) - gi_tiny->top;

        render::set_text_align(text_align::RIGHT);
        render::draw_text(tiny_font, {50, y_tiny}, TEXT_LAYER, rank_text_.c_str());

        render::set_text_align(text_align::LEFT);
        render::draw_text(tiny_font, {68, y_tiny}, TEXT_LAYER, name_.c_str());
    }
}

class local_leaderboard : public leaderboard_page
{
public:
    local_leaderboard(const std::string &title, const std::string &cache_path);

    void draw(float alpha) const override;
    bool async_check_hiscore(leaderboard_event_listener *listener, int score) override;
    bool async_insert_hiscore(leaderboard_event_listener *listener, const wchar_t *name, int score) override;
};

class net_leaderboard : public leaderboard_page
{
public:
    net_leaderboard(const std::string &title, const std::string &cache_path, const std::string &url);

    void reset() override;

    void on_request_error();
    void on_request_completed(int status, const char *content, size_t content_len);

    void draw(float alpha) const override;
    bool async_check_hiscore(leaderboard_event_listener *listener, int score) override;
    bool async_insert_hiscore(leaderboard_event_listener *listener, const wchar_t *name, int score) override;

private:
    static const time_t MAX_CACHE_AGE = 30 * 60; // in seconds

    void start_loading();

    bool need_refresh() const;
    bool cache_expired() const;

    void draw_loading_message(const g2d::mat4 &mat, float alpha) const;
    void draw_error_message(const g2d::mat4 &mat, float alpha) const;

    void answer_hiscore_requests();

    std::string url_;

    enum state
    {
        STATE_NONE,
        STATE_LOADING,
        STATE_DISPLAYING,
        STATE_ERROR,
    } state_;

    std::list<std::pair<leaderboard_event_listener *, int>> check_hiscore_requests_;
};

leaderboard_page::leaderboard_page(const std::string &title, const std::string &cache_path)
    : cache_path_(cache_path)
    , title_text_(title.begin(), title.end())
{
}

leaderboard_page::~leaderboard_page()
{
    clear_items();
}

void leaderboard_page::reset()
{
    y_offset_ = 0;
    speed_ = 0;
    tics_ = 0;
}

bool leaderboard_page::is_hiscore(int score) const
{
    return items_.size() < MAX_HISCORES || score > items_.back()->get_score();
}

bool leaderboard_page::load_cache()
{
    if (FILE *in = fopen(cache_path_.c_str(), "r")) {
        char line[512];

        int index = 1;

        while (fgets(line, sizeof line, in)) {
            char *name = strtok(line, ":");
            int score = atoi(strtok(nullptr, "\n"));

            // XXX: should check if list is ordered
            items_.push_back(new item(index, utf8_to_wchar(name), score, false));
            ++index;
        }

        fclose(in);

        return true;
    }

    return false;
}

void leaderboard_page::save_cache() const
{
    if (FILE *out = fopen(cache_path_.c_str(), "w")) {
        std::for_each(items_.begin(), items_.end(), [=](const item *p) {
            unsigned char *name_utf8 = wchar_to_utf8(p->get_name().c_str());
            fprintf(out, "%s:%d\n", name_utf8, p->get_score());
            delete[] name_utf8;
        });

        fclose(out);
    }
}

void leaderboard_page::draw_title(float alpha) const
{
    const auto text_color = g2d::rgba{1., 1., 1., alpha};
    const auto outline_color = g2d::rgba{.5, 0., 0., alpha};

    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    render::push_matrix();
    render::translate(.5 * window_width, window_height - .5 * TITLE_HEIGHT - 14);
    render::set_text_align(text_align::CENTER);
    render::draw_text(get_font(font::medium), {}, 100,
            outline_color, text_color, outline_color, text_color,
            title_text_.c_str());
    render::pop_matrix();
}

void leaderboard_page::draw_items(float alpha) const
{
    const float top_y = window_height - TITLE_HEIGHT;
    const float total_height = items_.size() * item::HEIGHT;

    render::set_scissor_test(true);
    render::set_scissor_box(0, 0, viewport_width, top_y * viewport_height / window_height);

    float y = top_y + y_offset_;
    if (y < top_y)
        y = top_y;
    else if (y > total_height)
        y = total_height;

    for (auto item : items_) {
        if (y - item::HEIGHT < top_y)
        {
            render::push_matrix();
            render::translate(0, y);
            item->draw(alpha);
            render::pop_matrix();
        }

        y -= item::HEIGHT;

        if (y < 0)
            break;
    }

    render::set_scissor_test(false);
}

void leaderboard_page::update(uint32_t dt)
{
    tics_ += dt;

    static const float MIN_SPEED = .5 / MS_PER_TIC;

    if (fabs(speed_) > MIN_SPEED) {
        update_y_offset(speed_ * dt);
        speed_ -= dt * .005 * speed_;
    } else {
        speed_ = 0.;
    }
}

void leaderboard_page::update_y_offset(float dy)
{
    const float top_y = window_height - TITLE_HEIGHT;
    const float total_height = items_.size() * item::HEIGHT;

    float next_y_offset = y_offset_ + dy;

    if (next_y_offset < 0)
        y_offset_ = 0;
    else if (next_y_offset > total_height - top_y)
        y_offset_ = total_height - top_y;
    else
        y_offset_ = next_y_offset;
}

void leaderboard_page::on_drag_start()
{
    speed_ = total_drag_dy_ = 0;
    touch_start_tic_ = tics_;
}

void leaderboard_page::on_drag_end()
{
    speed_ = total_drag_dy_ / (tics_ - touch_start_tic_);
}

void leaderboard_page::on_drag(float dy)
{
    total_drag_dy_ += dy;
    update_y_offset(dy);
}

void leaderboard_page::clear_items()
{
    std::for_each(items_.begin(), items_.end(), [](item *p) { delete p; });

    items_.clear();
}

//
//  l o c a l _ l e a d e r b o a r d
//

local_leaderboard::local_leaderboard(const std::string &title, const std::string &cache_path)
    : leaderboard_page(title, cache_path)
{
    if (!load_cache()) {
        for (int i = 0; i < MAX_HISCORES; i++)
            items_.push_back(new item(i + 1, xwcsdup(i % 2 ? L"foo" : L"bar"), MAX_HISCORES - i, false));
    }
}

void local_leaderboard::draw(float alpha) const
{
    draw_title(alpha);
    draw_items(alpha);
}

bool local_leaderboard::async_check_hiscore(leaderboard_event_listener *listener, int score)
{
    listener->on_check_hiscore_response(true, is_hiscore(score));
    return true;
}

bool local_leaderboard::async_insert_hiscore(leaderboard_event_listener *listener, const wchar_t *name, int score)
{
    auto it = items_.begin();

    for (; it != items_.end(); ++it) {
        if ((*it)->get_score() < score)
            break;
    }

    if (it != items_.end()) {
        it = items_.insert(it, new item((*it)->get_rank(), xwcsdup(name), score, true));

        for (++it; it != items_.end(); ++it)
            (*it)->set_rank((*it)->get_rank() + 1);

        save_cache();
    }

    return true;
}

//
//   n e t _ l e a d e r b o a r d
//

net_leaderboard::net_leaderboard(const std::string &title, const std::string &cache_path, const std::string &url)
    : leaderboard_page(title, cache_path)
    , url_(url)
    , state_(STATE_NONE)
{
}

void net_leaderboard::reset()
{
    leaderboard_page::reset();

    if (need_refresh())
        start_loading();
}

void net_leaderboard::draw(float alpha) const
{
#ifdef FIX_ME
    switch (state_) {
        case STATE_LOADING:
            draw_loading_message(mat, alpha);
            break;

        case STATE_ERROR:
            draw_error_message(mat, alpha);
            break;

        case STATE_DISPLAYING:
            draw_title(mat, alpha);
            draw_items(mat, alpha);
            break;

        default:
            assert(0);
    }
#endif
}

bool net_leaderboard::need_refresh() const
{
    return state_ == STATE_NONE || state_ == STATE_ERROR || (state_ == STATE_DISPLAYING && cache_expired());
}

bool net_leaderboard::cache_expired() const
{
    struct stat sb;
    if (stat(cache_path_.c_str(), &sb) < 0)
        return true;

    time_t now;
    time(&now);

    return now - sb.st_ctime > MAX_CACHE_AGE;
}

void net_leaderboard::start_loading()
{
    if (!cache_expired() && load_cache()) {
        state_ = STATE_DISPLAYING;
    } else {
        auto req = new http_request;

        state_ = STATE_LOADING;

        bool ok = req->get(url_.c_str(), [this](const http_response &resp) {
            if (resp.success) {
                on_request_completed(resp.status, resp.content, resp.content_len);
            } else {
                on_request_error();
            }
        });

        if (!ok) {
            delete req;
            state_ = STATE_ERROR;
        } else {
            kasui::get_instance().add_http_request(req);
        }
    }
}

void net_leaderboard::answer_hiscore_requests()
{
    assert(state_ == STATE_DISPLAYING || state_ == STATE_ERROR);

    std::for_each(check_hiscore_requests_.begin(), check_hiscore_requests_.end(),
                  [this](std::pair<leaderboard_event_listener *, int> &req) {
                      if (state_ == STATE_ERROR)
                          req.first->on_check_hiscore_response(false, false);
                      else
                          req.first->on_check_hiscore_response(true, is_hiscore(req.second));
                  });

    check_hiscore_requests_.clear();
}

void net_leaderboard::on_request_error()
{
    state_ = STATE_ERROR;
    answer_hiscore_requests();
}

void net_leaderboard::on_request_completed(int status, const char *content, size_t content_len)
{
    if (status != 200) {
        state_ = STATE_ERROR;
        answer_hiscore_requests();
        return;
    }

    clear_items();

    int score = 0;
    std::string name;
    bool highlight = false;

    int index = 1;
    int state = 0;

    for (const char *p = content; p != &content[content_len]; p++) {
        char ch = *p;

        switch (state) {
            case 0:
                if (ch == ':') {
                    state = 1;
                } else if (ch >= '0' && ch <= '9') {
                    score = score * 10 + ch - '0';
                }
                break;

            case 1:
                if (ch == ':') {
                    state = 2;
                } else {
                    name.push_back(ch);
                }
                break;

            case 2:
                if (ch == '\n') {
                    items_.push_back(new item(index, utf8_to_wchar(name.c_str()), score, highlight));
                    ++index;

                    score = 0;
                    name.clear();
                    highlight = false;

                    state = 0;
                } else if (ch == '1') {
                    highlight = true;
                }
                break;

            default:
                assert(0);
        }
    }

    save_cache();

    state_ = STATE_DISPLAYING;
    answer_hiscore_requests();
}

void net_leaderboard::draw_loading_message(const g2d::mat4 &mat, float alpha) const
{
    draw_message(mat, alpha, L"loading leaderboard...");
}

void net_leaderboard::draw_error_message(const g2d::mat4 &mat, float alpha) const
{
    draw_message(mat * g2d::mat4::translation(0, 20, 0), alpha, L"something went wrong!");
    draw_message(mat * g2d::mat4::translation(0, -48, 0), alpha, L"please check your");
    draw_message(mat * g2d::mat4::translation(0, -96, 0), alpha, L"network settings");
}

bool net_leaderboard::async_check_hiscore(leaderboard_event_listener *listener, int score)
{
    if (need_refresh())
        start_loading();

    switch (state_) {
        case STATE_LOADING:
            check_hiscore_requests_.push_back(std::make_pair(listener, score));
            return true;

        case STATE_DISPLAYING:
            listener->on_check_hiscore_response(true, is_hiscore(score));
            return true;

        case STATE_ERROR:
        default:
            // no connection?
            return false;
    }
}

bool net_leaderboard::async_insert_hiscore(leaderboard_event_listener *listener, const wchar_t *name, int score)
{
    assert(state_ == STATE_DISPLAYING);

    std::stringstream ss;

    ss << url_ << "?score=" << score << "&name=";

    ss << std::hex;

    auto name_utf8 = wchar_to_utf8(name);
    for (auto *p = name_utf8; *p; p++)
        ss << '%' << static_cast<int>(*p);
    delete[] name_utf8;

    auto req = new http_request;

    state_ = STATE_LOADING;

    bool ok = req->get(url_.c_str(), [this](const http_response &resp) {
        if (resp.success) {
            on_request_completed(resp.status, resp.content, resp.content_len);
        } else {
            on_request_error();
        }
    });

    if (!ok) {
        delete req;
        state_ = STATE_ERROR;
        return false;
    } else {
        kasui::get_instance().add_http_request(req);
        return true;
    }
}

leaderboard_page &get_net_leaderboard()
{
    static net_leaderboard instance("top scores", "leaderboard-net", "http://games.fzort.org/kasui/leaderboard.php");
    // static net_leaderboard instance("top scores", "leaderboard-net",
    // "http://localhost/leaderboard.php");
    return instance;
}

leaderboard_page &get_local_leaderboard()
{
    static local_leaderboard instance("top scores", "leaderboard-local");
    return instance;
}
