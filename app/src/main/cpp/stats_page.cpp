#include "stats_page.h"

#include "guava2d/rgb.h"
#include "guava2d/texture_manager.h"
#include "guava2d/vec3.h"
#include "guava2d/xwchar.h"

#include "background.h"
#include "common.h"
#include "jukugo.h"
#include "kanji_info.h"
#include "line_splitter.h"
#include "main_menu.h"
#include "pause_button.h"
#include "sprite_manager.h"
#include "theme.h"
#include "render.h"
#include "program_manager.h"
#include "fonts.h"

#include <algorithm>
#include <set>
#include <vector>
#include <cassert>

extern state *get_main_menu_state();

static constexpr int FRAME_LAYER = 99;
static constexpr int TEXT_LAYER = 100;

class stats_page_item
{
public:
    stats_page_item(const g2d::texture *frame_texture);
    virtual ~stats_page_item() {}

    virtual void draw(float alpha) const = 0;
    virtual int get_height() const = 0;
    virtual int get_left_border() const = 0;
    virtual float get_frame_alpha() const = 0;

    virtual void reset_contents() = 0;

    void draw_frame(float alpha) const;

private:
    const g2d::texture *frame_texture_;
};

class kanji_info_item : public stats_page_item
{
public:
    kanji_info_item(const kanji_info *kanji);

    void draw(float alpha) const override;

    int get_height() const override { return 116; }

    int get_left_border() const override { return 0; }

    float get_frame_alpha() const override { return .75; }

    void reset_contents() override {}

private:
    const g2d::program *program_;
    const kanji_info *kanji_;
    std::wstring kanji_text_;
};

class jukugo_info_item : public stats_page_item
{
public:
    jukugo_info_item(const jukugo *jukugo);

    void draw(float alpha) const override;

    int get_height() const override { return height_; }

    int get_left_border() const override { return 20; }

    float get_frame_alpha() const override { return .5; }

    void reset_contents() override;

private:
    void draw_text(const g2d::mat4 &proj_modelview) const;

    const jukugo *jukugo_;

    int height_;
    std::vector<std::wstring> meaning_text_;
    std::wstring hits_text_;

    static constexpr int MAX_MEANING_WIDTH = 360;
    static constexpr int LINE_HEIGHT = 30;
};

class stats_page
{
public:
    stats_page(int level, float top_y);

    void reset();
    void reset_contents();

    void draw(float alpha) const;
    void update(uint32_t dt);

    void on_drag_start();
    void on_drag_end();
    void on_drag(float dy);

    static const int TITLE_HEIGHT = 96;

private:
    void draw_title(float alpha) const;
    void update_y_offset(float dy);

    std::vector<stats_page_item *> items_;
    std::wstring title_text_;
    float y_offset_;
    float top_y_;
    int touch_start_tic_;
    float total_drag_dy_;
    float speed_;
    int total_height_;
    int tics_;
};

class stats_state_impl
{
public:
    stats_state_impl();

    void reset();
    void redraw() const;
    void update(uint32_t dt);
    void on_touch_down(float x, float y);
    void on_touch_up();
    void on_touch_move(float x, float y);
    void on_back_key();
    void on_menu_key();

private:
    void draw_pages(float x_offset) const;
    void draw_page(const stats_page *page, float x_offset) const;
    int get_next_level() const;

    int cur_level_;
    int title_height_;
    stats_page *level_stats_[NUM_LEVELS];
    pause_button pause_button_;
    float last_touch_x_, last_touch_y_;
    float x_offset_;

    enum
    {
        INTRO,
        ACTIVE,
        DRAG_START,
        DRAG_HORIZ,
        DRAG_VERT,
        TRANSITION,
        OUTRO,
    } cur_state_;

    int state_tics_;

    static const int TRANSITION_TICS = 10 * MS_PER_TIC;
    static const int INTRO_TICS = 15 * MS_PER_TIC;
    static const int OUTRO_TICS = 15 * MS_PER_TIC;
};

stats_page_item::stats_page_item(const g2d::texture *frame_texture)
    : frame_texture_(frame_texture)
{
}

void stats_page_item::draw_frame(float alpha) const
{
    render::set_blend_mode(blend_mode::ALPHA_BLEND);
    render::set_color({1, 1, 1, alpha});

    static const int border_radius = 16;

    const int height = get_height(), left_border = get_left_border();

    const float x0 = left_border, x1 = left_border + border_radius, x2 = window_width - border_radius,
                x3 = window_width;
    const float y0 = 0, y1 = -border_radius, y2 = -(height - border_radius), y3 = -height;

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

kanji_info_item::kanji_info_item(const kanji_info *kanji)
    : stats_page_item(g2d::load_texture("images/b-button-border.png"))
    , program_{load_program("shaders/sprite.vert", "shaders/text_no_outline.frag")}
    , kanji_{kanji}
{
    kanji_text_.push_back(kanji->code);
}

void kanji_info_item::draw(float alpha) const
{
    draw_frame(alpha);

    const int height = get_height();

    const auto draw_text = [this, height]() {
        render::set_text_align(text_align::LEFT);
        render::draw_text(program_, get_font(font::large), {10.f, -.5f * height - 28.f}, TEXT_LAYER, kanji_text_.c_str());
        render::draw_text(program_, get_font(font::micro), {110.f, -.5f * height + 6.f}, TEXT_LAYER, kanji_->on);
        render::draw_text(program_, get_font(font::micro), {110.f, -.5f * height - 24.f}, TEXT_LAYER, kanji_->kun);

        render::set_text_align(text_align::RIGHT);
        render::draw_text(program_, get_font(font::small), {window_width - 20.f, -.5f * height - 12.f}, TEXT_LAYER, kanji_->meaning);
    };

    render::set_blend_mode(blend_mode::ALPHA_BLEND);

    render::set_color({0, 0, 0, .5f * alpha});
    render::push_matrix();
    render::translate(3, -3);
    draw_text();
    render::pop_matrix();

    render::set_color({1, 1, 1, alpha});
    draw_text();
}

jukugo_info_item::jukugo_info_item(const jukugo *jukugo)
    : stats_page_item(g2d::load_texture("images/w-button-border.png"))
    , jukugo_(jukugo)
{
    static wchar_t meaning_buf[512];
    xswprintf(meaning_buf, L"(%s) %s", jukugo->reading, jukugo->eigo);

    {
        line_splitter ls(get_font(font::micro), meaning_buf);
        std::wstring line;
        while (line = ls.next_line(MAX_MEANING_WIDTH), !line.empty())
            meaning_text_.push_back(line);
    }

    static constexpr int MIN_HEIGHT = 72;
    height_ = std::max(MIN_HEIGHT, 14 + static_cast<int>(meaning_text_.size()) * LINE_HEIGHT);

    reset_contents();
}

void jukugo_info_item::reset_contents()
{
    wchar_t hits_buf[80];
    xswprintf(hits_buf, L"%d", jukugo_->hits);
    hits_text_ = hits_buf;
}

void jukugo_info_item::draw(float alpha) const
{
    draw_frame(alpha);

    render::set_blend_mode(blend_mode::INVERSE_BLEND);
    render::set_color({alpha, alpha, alpha, 1.f});

    render::set_text_align(text_align::LEFT);
    render::draw_text(get_font(font::medium), {30.f, -.5f * height_ - 16.f}, TEXT_LAYER, jukugo_->kanji);

    wchar_t *line;
    int y = -.5 * height_ + .5 * meaning_text_.size() * LINE_HEIGHT + 2;

    for (const auto& line : meaning_text_) {
        render::draw_text(get_font(font::micro), {160.f, y - 22.f}, TEXT_LAYER, line.c_str());
        y -= LINE_HEIGHT;
    }

    render::set_text_align(text_align::RIGHT);
    render::draw_text(get_font(font::micro), {window_width - 20.f, -.5f * height_ - 8.f}, TEXT_LAYER, hits_text_.c_str());
}

stats_page::stats_page(int level, float top_y)
    : top_y_(top_y)
    , total_height_(0)
{
    struct kanji_jukugo
    {
        kanji_jukugo(const kanji_info *ki)
            : ki(ki)
        {
        }

        const kanji_info *ki;
        std::vector<const jukugo *> jukugo_list;
    };

    std::vector<kanji_jukugo> kanji_jukugos;

    std::for_each(kanji_info_list.begin(), kanji_info_list.end(), [&](const kanji_info *ki) {
        if (ki->level == level)
            kanji_jukugos.push_back(kanji_jukugo(ki));
    });

    for (const auto& jukugo : jukugo_list) {
        if (jukugo.level != level)
            continue;

        kanji_jukugo *p = nullptr;

        for (auto &q : kanji_jukugos) {
            if (q.ki->code == jukugo.kanji[0] || q.ki->code == jukugo.kanji[1]) {
                if (p == nullptr || q.jukugo_list.size() < p->jukugo_list.size())
                    p = &q;
            }
        }

        assert(p);

        p->jukugo_list.push_back(&jukugo);
    }

    for (auto &kanji_jukugo : kanji_jukugos) {
        items_.push_back(new kanji_info_item(kanji_jukugo.ki));

        for (auto j = kanji_jukugo.jukugo_list.begin(), end = kanji_jukugo.jukugo_list.end(); j != end; j++)
            items_.push_back(new jukugo_info_item(*j));
    }

    for (auto &item : items_)
        total_height_ += item->get_height();

    reset();

    // title

    wchar_t title_buf[80];
    xswprintf(title_buf, L"level %d", level + 1);
    title_text_ = title_buf;
}

void stats_page::reset()
{
    y_offset_ = 0;
    speed_ = 0;
    tics_ = 0;
}

void stats_page::reset_contents()
{
    for (auto &item : items_)
        item->reset_contents();
}

void stats_page::draw(float alpha) const
{
    render::set_scissor_test(true);

    float y = top_y_ + y_offset_;
    if (y < top_y_)
        y = top_y_;
    else if (y > total_height_)
        y = total_height_;

    for (auto item : items_) {
        if (y - item->get_height() < top_y_) {
            render::push_matrix();
            render::translate(0, y);
            item->draw(alpha);
            render::pop_matrix();
        }

        y -= item->get_height();

        if (y < 0)
            break;
    }

    render::set_scissor_test(false);

    draw_title(alpha);
}

void stats_page::draw_title(float alpha) const
{
    render::set_blend_mode(blend_mode::INVERSE_BLEND);
    render::set_color({alpha, alpha, alpha, 1.f});
    render::set_text_align(text_align::CENTER);
    render::draw_text(get_font(font::medium), {.5f * window_width, window_height - .5f * TITLE_HEIGHT - 14.f}, TEXT_LAYER, title_text_.c_str());
}

void stats_page::update(uint32_t dt)
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

void stats_page::on_drag_start()
{
    speed_ = total_drag_dy_ = 0;
    touch_start_tic_ = tics_;
}

void stats_page::on_drag_end()
{
    speed_ = total_drag_dy_ / (tics_ - touch_start_tic_);
}

void stats_page::update_y_offset(float dy)
{
    float next_y_offset = y_offset_ + dy;

    if (next_y_offset < 0)
        y_offset_ = 0;
    else if (next_y_offset > total_height_ - top_y_)
        y_offset_ = total_height_ - top_y_;
    else
        y_offset_ = next_y_offset;
}

void stats_page::on_drag(float dy)
{
    total_drag_dy_ += dy;
    update_y_offset(dy);
}

stats_state_impl::stats_state_impl()
    : pause_button_(window_width - pause_button::SIZE, window_height - pause_button::SIZE)
{
    pause_button_.set_callback([this] { on_back_key(); });

    for (int i = 0; i < NUM_LEVELS; i++)
        level_stats_[i] = new stats_page(i, window_height - stats_page::TITLE_HEIGHT);

    reset();
}

void stats_state_impl::reset()
{
    cur_level_ = 0;

    x_offset_ = 0;
    cur_state_ = INTRO;

    state_tics_ = 0;

    for (auto &level_stat : level_stats_) {
        level_stat->reset_contents();
        level_stat->reset();
    }
}

void stats_state_impl::redraw() const
{
    get_main_menu_state()->redraw(); // draw main menu background

    const auto top_y = window_height - stats_page::TITLE_HEIGHT;
    render::set_scissor_box(0, 0, viewport_width, top_y * viewport_height / window_height);

    float pause_alpha = 1;

    switch (cur_state_) {
        case ACTIVE:
        case DRAG_START:
        case DRAG_VERT:
            level_stats_[cur_level_]->draw(1);
            break;

        case DRAG_HORIZ:
            draw_pages(x_offset_);
            break;

        case TRANSITION: {
            float t = static_cast<float>(state_tics_) / TRANSITION_TICS;
            t *= t;

            float dest_x_offset = -(get_next_level() - cur_level_) * window_width;
            draw_pages((1. - t) * x_offset_ + t * dest_x_offset);
        } break;

        case INTRO: {
            float t = static_cast<float>(state_tics_) / INTRO_TICS;
            pause_alpha = t;
            t *= t;

            draw_page(level_stats_[cur_level_], (1. - t) * window_width);
        } break;

        case OUTRO: {
            float t = static_cast<float>(state_tics_) / OUTRO_TICS;
            pause_alpha = 1. - t;
            t *= t;

            draw_page(level_stats_[cur_level_], t * window_width);
        } break;

        default:
            break;
    }

    pause_button_.draw(pause_alpha);
}

void stats_state_impl::draw_pages(float x_offset) const
{
    draw_page(level_stats_[cur_level_], x_offset);

    const stats_page *next_page = nullptr;
    float next_x_offset;

    if (x_offset < 0) {
        if (cur_level_ < NUM_LEVELS - 1) {
            next_page = level_stats_[cur_level_ + 1];
            next_x_offset = window_width + x_offset;
        }
    } else {
        if (cur_level_ > 0) {
            next_page = level_stats_[cur_level_ - 1];
            next_x_offset = x_offset - window_width;
        }
    }

    if (next_page)
        draw_page(next_page, next_x_offset);
}

void stats_state_impl::draw_page(const stats_page *page, float x_offset) const
{
    float alpha = 1. - fabs(x_offset) / window_width;

    render::push_matrix();
    render::translate(x_offset, 0);
    page->draw(alpha);
    render::pop_matrix();
}

void stats_state_impl::update(uint32_t dt)
{
    get_main_menu_state()->update(dt); // update main menu background

    level_stats_[cur_level_]->update(dt);

    switch (cur_state_) {
        case INTRO:
            if ((state_tics_ += dt) >= INTRO_TICS)
                cur_state_ = ACTIVE;
            break;

        case OUTRO:
            if ((state_tics_ += dt) >= OUTRO_TICS) {
                auto *main_menu = dynamic_cast<main_menu_state *>(get_prev_state());
                if (main_menu) {
                    main_menu->show_background();
                    main_menu->show_more_menu();
                } else {
                    get_prev_state()->reset();
                }

                pop_state();
            }
            break;

        case TRANSITION:
            if ((state_tics_ += dt) >= TRANSITION_TICS) {
                cur_level_ = get_next_level();
                cur_state_ = ACTIVE;
            }
            break;

        default:
            break;
    }
}

int stats_state_impl::get_next_level() const
{
    if (fabs(x_offset_) > .25 * window_width) {
        if (x_offset_ < 0) {
            if (cur_level_ < NUM_LEVELS - 1)
                return cur_level_ + 1;
        } else {
            if (cur_level_ > 0)
                return cur_level_ - 1;
        }
    }

    return cur_level_;
}

void stats_state_impl::on_touch_down(float x, float y)
{
    if (pause_button_.on_touch_down(x, y))
        return;

    switch (cur_state_) {
        case ACTIVE:
            cur_state_ = DRAG_START;
            last_touch_x_ = x;
            last_touch_y_ = y;
            x_offset_ = 0;
            break;

        default:
            break;
    }
}

void stats_state_impl::on_touch_up()
{
    if (pause_button_.on_touch_up())
        return;

    switch (cur_state_) {
        case DRAG_VERT:
            level_stats_[cur_level_]->on_drag_end();
            cur_state_ = ACTIVE;
            break;

        case DRAG_HORIZ:
            state_tics_ = 0;
            cur_state_ = TRANSITION;
            break;

        default:
            break;
    }
}

void stats_state_impl::on_touch_move(float x, float y)
{
    if (pause_button_.on_touch_down(x, y))
        return;

    const float EPSILON = 8;

    const float dx = x - last_touch_x_;
    const float dy = y - last_touch_y_;

    switch (cur_state_) {
        case DRAG_START:
            if (fabs(dy) > EPSILON || fabs(dx) > EPSILON) {
                if (fabs(dy) > fabs(dx)) {
                    level_stats_[cur_level_]->on_drag_start();
                    level_stats_[cur_level_]->on_drag(dy);
                    cur_state_ = DRAG_VERT;
                } else {
                    x_offset_ = dx;
                    cur_state_ = DRAG_HORIZ;
                }
            }
            break;

        case DRAG_HORIZ:
            x_offset_ += dx;
            last_touch_x_ = x;
            last_touch_y_ = y;
            break;

        case DRAG_VERT:
            level_stats_[cur_level_]->on_drag(dy);
            last_touch_x_ = x;
            last_touch_y_ = y;
            break;

        default:
            break;
    }
}

void stats_state_impl::on_back_key()
{
    if (cur_state_ == ACTIVE) {
        auto *main_menu = dynamic_cast<main_menu_state *>(get_prev_state());
        if (main_menu)
            main_menu->show_background();

        cur_state_ = OUTRO;
        state_tics_ = 0;
    }
}

void stats_state_impl::on_menu_key()
{
}

stats_page_state::stats_page_state()
    : impl_(new stats_state_impl)
{
}

stats_page_state::~stats_page_state()
{
    delete impl_;
}

void stats_page_state::reset()
{
    impl_->reset();
}

void stats_page_state::redraw() const
{
    impl_->redraw();
}

void stats_page_state::update(uint32_t dt)
{
    impl_->update(dt);
}

void stats_page_state::on_touch_down(float x, float y)
{
    impl_->on_touch_down(x, y);
}

void stats_page_state::on_touch_up()
{
    impl_->on_touch_up();
}

void stats_page_state::on_touch_move(float x, float y)
{
    impl_->on_touch_move(x, y);
}

void stats_page_state::on_back_key()
{
    impl_->on_back_key();
}

void stats_page_state::on_menu_key()
{
    impl_->on_menu_key();
}
