#include <cerrno>
#include <cstdio>
#include <cstring>

#include <sys/stat.h>
#include <time.h>

#include <algorithm>
#include <list>

#include <iomanip>
#include <sstream>

#include <guava2d/draw_queue.h>
#include <guava2d/font_manager.h>
#include <guava2d/rgb.h>
#include <guava2d/texture_manager.h>
#include <guava2d/vertex_array.h>
#include <guava2d/xwchar.h>

#include "common.h"
#include "http_request.h"
#include "kasui.h"
#include "leaderboard_page.h"
#include "log.h"
#include "program_registry.h"
#include "utf8.h"

void draw_message(const g2d::mat4 &mat, float alpha, const wchar_t *text)
{
    const g2d::font *small_font = g2d::font_manager::get_instance().load("fonts/tiny");

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
}

namespace {

class score_text
{
public:
    score_text(const g2d::font *font);

    void initialize(int value);

    float get_width() const { return width_; }

    void draw(const g2d::mat4 &mat, float alpha) const;

private:
    void add_digit(wchar_t ch);
    void format_thousands(int n);

    const g2d::font *font_;
    g2d::indexed_vertex_array<GLushort, g2d::vertex::attrib<GLfloat, 2>, g2d::vertex::attrib<GLfloat, 2>,
                              g2d::vertex::attrib<GLfloat, 1>>
        va_;
    float width_;
};

score_text::score_text(const g2d::font *font)
    : font_(font)
    , width_(0)
{
}

void score_text::add_digit(wchar_t ch)
{
    const g2d::glyph_info *g = font_->find_glyph(ch);

    const g2d::vec2 &t0 = g->texuv[0];
    const g2d::vec2 &t1 = g->texuv[1];
    const g2d::vec2 &t2 = g->texuv[2];
    const g2d::vec2 &t3 = g->texuv[3];

    // XXX for now
    const float x = width_;
    const float y = 0;

    const float x_left = x + g->left;
    const float x_right = x_left + g->width;
    const float y_top = y + g->top;
    const float y_bottom = y_top - g->height;

    const g2d::vec2 p0(x_left, y_top);
    const g2d::vec2 p1(x_right, y_top);
    const g2d::vec2 p2(x_right, y_bottom);
    const g2d::vec2 p3(x_left, y_bottom);

    const int vert_index = va_.get_num_verts();

    va_ << p0.x, p0.y, t0.x, t0.y, 0;
    va_ << p1.x, p1.y, t1.x, t1.y, 0;
    va_ << p2.x, p2.y, t2.x, t2.y, 1;
    va_ << p3.x, p3.y, t3.x, t3.y, 1;

    va_ < vert_index + 0, vert_index + 1, vert_index + 2, vert_index + 2, vert_index + 3, vert_index + 0;

    width_ += g->advance_x;
}

void score_text::format_thousands(int n)
{
    if (n >= 1000) {
        format_thousands(n / 1000);

        const int m = n % 1000;

        add_digit(',');
        add_digit('0' + (m / 100));
        add_digit('0' + (m / 10) % 10);
        add_digit('0' + m % 10);
    } else {
        if (n >= 100)
            add_digit('0' + (n / 100));
        if (n >= 10)
            add_digit('0' + (n / 10) % 10);
        add_digit('0' + n % 10);
    }
}

void score_text::initialize(int score)
{
    va_.reset();
    width_ = 0;

    format_thousands(score);
}

void score_text::draw(const g2d::mat4 &mat, float alpha) const
{
    font_->get_texture()->bind();

    const g2d::rgb top_color(200, 200, 255);
    const g2d::rgb bottom_color(120, 120, 255);

    const g2d::rgb top_color_text = top_color * (1. / 255);
    const g2d::rgb bottom_color_text = bottom_color * (1. / 255);
    const g2d::rgb top_color_outline = .5 * top_color_text;
    const g2d::rgb bottom_color_outline = .5 * bottom_color_text;

    program_intro_text &prog = get_program_instance<program_intro_text>();
    prog.use();
    // prog.set_proj_modelview_matrix(mat*g2d::mat4::translation(-.5*width_, 0,
    // 0));
    prog.set_proj_modelview_matrix(mat);
    prog.set_texture(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    prog.set_top_color_text(g2d::rgba(top_color_text, alpha));
    prog.set_bottom_color_text(g2d::rgba(bottom_color_text, alpha));
    prog.set_top_color_outline(g2d::rgba(top_color_outline, alpha));
    prog.set_bottom_color_outline(g2d::rgba(bottom_color_outline, alpha));

    va_.draw(GL_TRIANGLES);
}

} // namespace

class item
{
public:
    item(int rank, wchar_t *name, int score, bool highlight);
    ~item();

    void draw(const g2d::mat4 &mat, float alpha) const;

    int get_rank() const { return rank_; }

    int get_score() const { return score_; }

    wchar_t *get_name() const { return name_; }

    void set_rank(int rank)
    {
        rank_ = rank;
        initialize_text();
    }

    bool get_highlight() const { return highlight_; }

    static const int HEIGHT = 80;

private:
    void initialize_frame();
    void initialize_text();

    static const int BORDER_RADIUS = 16;

    int rank_;
    wchar_t *name_;
    int score_;
    bool highlight_;
    g2d::draw_queue text_; // , score_text_;
    const g2d::texture *frame_texture_;
    g2d::vertex_array_texuv frame_va_[3];
    score_text score_text_;
};

item::item(int rank, wchar_t *name, int score, bool highlight)
    : rank_(rank)
    , name_(name)
    , score_(score)
    , highlight_(highlight)
    , frame_texture_(g2d::texture_manager::get_instance().load("images/w-button-border.png"))
    , frame_va_{8, 8, 8}
    , score_text_(g2d::font_manager::get_instance().load("fonts/small"))
{
    initialize_text();
    initialize_frame();
}

item::~item()
{
    delete[] name_;
}

void item::initialize_text()
{
    const g2d::font *small_font = g2d::font_manager::get_instance().load("fonts/small");
    const g2d::font *tiny_font = g2d::font_manager::get_instance().load("fonts/tiny");

    auto gi_small = small_font->find_glyph(L'X');
    const float y_small = .5 * (-HEIGHT + gi_small->height) - gi_small->top;

    auto gi_tiny = tiny_font->find_glyph(L'X');
    const float y_tiny = .5 * (-HEIGHT + gi_tiny->height) - gi_tiny->top;

    text_.reset();

    text_
        .text_program(get_program_instance<program_texture_uniform_color>().get_raw())

        .push_matrix()
        .translate(50, y_tiny)
        .align_right()
        // .render_text(small_font, L"%d位", rank_)
        .render_text(tiny_font, L"%d", rank_)
        .pop_matrix()

        // .push_matrix()
        .translate(68, y_tiny)
        .align_left()
        .render_text(tiny_font, L"%s", name_);
// .pop_matrix()

#if 0
	score_text_.reset();

	score_text_
		.text_program(get_program_instance<program_timer_text>().get_raw())
		.translate(window_width - 28, y_small)
		.align_right()
		.render_text(small_font, L"%d", score_)
		.draw();
#else
    score_text_.initialize(score_);
#endif
}

void item::initialize_frame()
{
    const float x0 = 0;
    const float x1 = BORDER_RADIUS;

    const float x3 = window_width;
    const float x2 = x3 - BORDER_RADIUS;

    const float y0 = 0;
    const float y1 = -BORDER_RADIUS;

    const float y3 = -HEIGHT;
    const float y2 = y3 + BORDER_RADIUS;

    frame_va_[0] << x0, y0, 0, 0;
    frame_va_[0] << x0, y1, 0, .25;

    frame_va_[0] << x1, y0, .25, 0;
    frame_va_[0] << x1, y1, .25, .25;

    frame_va_[0] << x2, y0, .75, 0;
    frame_va_[0] << x2, y1, .75, .25;

    frame_va_[0] << x3, y0, 1, 0;
    frame_va_[0] << x3, y1, 1, .25;

    frame_va_[1] << x0, y1, 0, .25;
    frame_va_[1] << x0, y2, 0, .75;

    frame_va_[1] << x1, y1, .25, .25;
    frame_va_[1] << x1, y2, .25, .75;

    frame_va_[1] << x2, y1, .75, .25;
    frame_va_[1] << x2, y2, .75, .75;

    frame_va_[1] << x3, y1, 1, .25;
    frame_va_[1] << x3, y2, 1, .75;

    frame_va_[2] << x0, y2, 0, .75;
    frame_va_[2] << x0, y3, 0, 1;

    frame_va_[2] << x1, y2, .25, .75;
    frame_va_[2] << x1, y3, .25, 1;

    frame_va_[2] << x2, y2, .75, .75;
    frame_va_[2] << x2, y3, .75, 1;

    frame_va_[2] << x3, y2, 1, .75;
    frame_va_[2] << x3, y3, 1, 1;
}

void item::draw(const g2d::mat4 &mat, float alpha) const
{
    glEnable(GL_BLEND);

    // draw frame

    {
        g2d::rgba color;

        if (!highlight_) {
            color = g2d::rgba(1, 1, 1, alpha);
        } else {
            extern int total_tics;
            float t = .875 + .125 * sin(.2 * total_tics);
            color = g2d::rgba(1, t, t, alpha);
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        auto &prog = get_program_instance<program_texture_uniform_color>();
        prog.use();
        prog.set_proj_modelview_matrix(mat);
        prog.set_texture(0);
        prog.set_color(color);

        frame_texture_->bind();

        for (const auto &va : frame_va_)
            va.draw(GL_TRIANGLE_STRIP);
    }

#if 0
	{
	auto& prog = get_program_instance<program_timer_text>();
	prog.use();
	prog.set_proj_modelview_matrix(mat);
	prog.set_texture(0);

	prog.set_text_color(g2d::rgba(1., 1., 1., alpha));
	prog.set_outline_color(g2d::rgba(.5, 0., 0., alpha));

	score_text_.draw();
	}
#else
    score_text_.draw(mat * g2d::mat4::translation(window_width - 28 - score_text_.get_width(), -52.5, 0), alpha);
#endif

    {
        glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

        auto &prog = get_program_instance<program_texture_uniform_color>();
        prog.use();
        prog.set_proj_modelview_matrix(mat);
        prog.set_texture(0);
        prog.set_color(g2d::rgba(alpha, alpha, alpha, 1));

        text_.draw();
    }
}

class local_leaderboard : public leaderboard_page
{
public:
    local_leaderboard(const std::string &title, const std::string &cache_path);

    void draw(const g2d::mat4 &mat, float alpha) const override;
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

    void draw(const g2d::mat4 &mat, float alpha) const override;
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
{
    const g2d::font *font = g2d::font_manager::get_instance().load("fonts/medium");

    title_text_.reset();

    wchar_t *title_wchar = utf8_to_wchar(title.c_str());

    title_text_.text_program(get_program_instance<program_timer_text>().get_raw())
        .translate(.5 * window_width, window_height - .5 * TITLE_HEIGHT - 14)
        .align_center()
        .render_text(font, L"%s", title_wchar);

    delete[] title_wchar;
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
            unsigned char *name_utf8 = wchar_to_utf8(p->get_name());
            fprintf(out, "%s:%d\n", name_utf8, p->get_score());
            delete[] name_utf8;
        });

        fclose(out);
    }
}

void leaderboard_page::draw_title(const g2d::mat4 &mat, float alpha) const
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto &prog = get_program_instance<program_timer_text>();
    prog.use();
    prog.set_proj_modelview_matrix(mat);
    prog.set_texture(0);

    prog.set_text_color(g2d::rgba(1., 1., 1., alpha));
    prog.set_outline_color(g2d::rgba(.5, 0., 0., alpha));

    title_text_.draw();
}

void leaderboard_page::draw_items(const g2d::mat4 &mat, float alpha) const
{
    const float top_y = window_height - TITLE_HEIGHT;
    const float total_height = items_.size() * item::HEIGHT;

    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, viewport_width, top_y * viewport_height / window_height);

    float y = top_y + y_offset_;
    if (y < top_y)
        y = top_y;
    else if (y > total_height)
        y = total_height;

    for (auto item : items_) {
        if (y - item::HEIGHT < top_y)
            item->draw(mat * g2d::mat4::translation(0, y, 0), alpha);

        y -= item::HEIGHT;

        if (y < 0)
            break;
    }

    glDisable(GL_SCISSOR_TEST);
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

void local_leaderboard::draw(const g2d::mat4 &mat, float alpha) const
{
    draw_title(mat, alpha);
    draw_items(mat, alpha);
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

void net_leaderboard::draw(const g2d::mat4 &mat, float alpha) const
{
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
