#pragma once

#include "settings.h"

#include <guava2d/rgb.h>
#include <guava2d/vec2.h>

#include <memory>
#include <list>
#include <vector>

#include <cassert>
#include <cstdint>

#include <cassert>

class gradient;

namespace g2d {
class mat4;
class texture;
class program;
};

enum
{
    BAKUDAN_FLAG = 0x80,
};

class world;
class jukugo;

class falling_block
{
public:
    falling_block(world &w);

    void initialize();
    void draw() const;
    void update(uint32_t dt);

    bool on_left_pressed();
    bool on_right_pressed();
    bool on_up_pressed();
    bool on_down_pressed();

    const wchar_t *get_kanji_text() const;

    const wchar_t get_block_type(int index) const
    {
        assert(index == 0 || index == 1);
        return block_types[index] & ~BAKUDAN_FLAG;
    }

    void inactivate(); // on game over

    bool get_is_active() const { return is_active; }

    int block_types[2];

protected:
    void get_block_positions(g2d::vec2 &p0, g2d::vec2 &p1) const;
    void move_left();
    void move_right();

    bool is_active;

    enum
    {
        MOVING = 1, // move left or right
        SWAPPING = 2,
        DROPPING = 4,
        FADING_IN = 8,
    };

    int row, col;

    unsigned state_flags;

    int tics_to_drop; // tics before dropping to row below

    int swap_tics; // tics since swap animation started
    int move_tics; // tics since move animation started
    int drop_tics; // tics since drop animation started

    int move_dir; // -1: left, +1: right (if (state_flags|MOVING))

    bool is_moving() const { return (state_flags & MOVING); }
    void set_is_moving()
    {
        state_flags |= MOVING;
        move_tics = 0;
    }
    void unset_is_moving() { state_flags &= ~MOVING; }

    bool is_swapping() const { return (state_flags & SWAPPING); }
    void set_is_swapping()
    {
        state_flags |= SWAPPING;
        swap_tics = 0;
    }
    void unset_is_swapping() { state_flags &= ~SWAPPING; }

    bool is_dropping() const { return (state_flags & DROPPING); }
    void set_is_dropping()
    {
        state_flags |= DROPPING;
        drop_tics = 0;
    }
    void unset_is_dropping() { state_flags &= ~DROPPING; }

    bool is_fading_in() const { return (state_flags & FADING_IN); }
    void unset_is_fading_in() { state_flags &= ~FADING_IN; }

    bool can_drop() const;

    void drop();
    void drop_fast();

    world &world_;
};

class sprite
{
public:
    virtual ~sprite() = default;

    virtual bool update(uint32_t dt) = 0;
    virtual void draw() const = 0;
};

class world_event_listener
{
public:
    virtual ~world_event_listener(){};

    virtual void set_jukugo_left(int jukugo_left) = 0;
    virtual void set_score(int score) = 0;
    virtual void set_next_falling_blocks(wchar_t left, wchar_t right) = 0;
    virtual void on_falling_blocks_started() = 0;
};

class jukugo;

struct hint
{
    int block_type;
    int block_r, block_c;
    int match_r, match_c;
    const class jukugo *jukugo;
};

class world
{
public:
    world(int rows, int cols, int wanted_height);
    ~world();

    void set_level(int level, bool practice_mode, bool enable_hints);
    void set_enable_hints(bool enable);

    void set_row(int row_index, const wchar_t *row_kanji);
    void set_falling_blocks(wchar_t left, wchar_t right);

    void reset();
    void initialize_grid(int num_filled_rows);

    void draw() const;

    void update(uint32_t dt);
    void update_animations(uint32_t dt); // HACK: update() calls
                                         // update_animations(), so call one or
                                         // the other!

    bool on_left_pressed();
    bool on_right_pressed();
    bool on_up_pressed();
    bool on_down_pressed();

    float get_cell_size() const { return cell_size_; }

    int get_num_rows() const { return rows_; }

    int get_num_cols() const { return cols_; }

    int get_num_level_block_types() const { return num_level_block_types_; }

    float get_width() const { return cols_ * cell_size_; }

    float get_height() const { return rows_ * cell_size_; }

    int get_block_at(int row, int col) const { return grid_[row * cols_ + col]; }

    void set_block_at(int row, int col, int value) { grid_[row * cols_ + col] = value; }

    void set_block_kanji_at(int row, int col, wchar_t kanji);

    int get_block_type_at(int row, int col) const { return get_block_at(row, col) & ~BAKUDAN_FLAG; }

    int get_jukugo_left() const { return level_jukugo_left_; }

    int get_score() const { return score_; }

    bool is_in_falling_block_state() const { return cur_state_ == STATE_FALLING_BLOCK; }

    bool can_consume_gestures() const { return cur_state_ == STATE_FALLING_BLOCK || cur_state_ == STATE_HINT; }

    bool is_in_game_over_state() const { return cur_state_ == STATE_GAME_OVER; }

    bool is_in_level_completed_state() const { return cur_state_ == STATE_LEVEL_COMPLETED; }

    void set_game_over();

    const wchar_t *get_cur_falling_blocks() const;

    void set_theme_colors(const g2d::rgb &color, const g2d::rgb &opposite_color);
    void set_text_gradient(const gradient& g);

    void draw_block(int type, float x, float y, float alpha) const;
    void draw_block(int type, float x, float y, float alpha, const g2d::rgb &color) const;

    void spawn_drop_trail(int row, int col, int type);
    void spawn_dead_block_sprite(const g2d::vec2 &pos, int type);

    void set_event_listener(world_event_listener *listener) { event_listener_ = listener; }

private:
    bool get_hint(hint &h) const;

    void draw_background() const;
    void draw_blocks() const;
    void draw_flares() const;

    int get_col_height(int c) const;

    void initialize_dropping_blocks();
    bool find_matches();
    void solve_matches();
    bool has_hanging_blocks() const;
    bool update_dropping_blocks(uint32_t dt);
    void drop_hanging_blocks();
    bool is_game_over() const;
    bool is_level_completed() const;
    void next_falling_block();

    enum game_state
    {
        STATE_BEFORE_FALLING_BLOCK,
        STATE_HINT,
        STATE_FALLING_BLOCK,
        STATE_DROPPING_HANGING,
        STATE_FLARES,
        STATE_SOLVING_MATCHES,
        STATE_WAITING_CLIPS,
        STATE_LEVEL_COMPLETED,
        STATE_GAME_OVER,
    };

    void set_state(game_state state);

    void set_state_before_falling_block();
    void set_state_falling_block();
    void set_state_falling_block_or_hint();
    void set_state_dropping_hanging();

    void set_falling_block_on_listener(const falling_block &p) const;

    bool practice_mode_;
    bool enable_hints_;

    int rows_, cols_;
    std::vector<int> grid_;
    std::vector<bool> matches_;
    int num_level_block_types_;
    float cell_size_;
    g2d::rgb theme_color_, theme_opposite_color_;
    gradient text_gradient_;

    const g2d::texture *blocks_texture_;
    const g2d::texture *flare_texture_;

    const g2d::program *program_grid_background_;

    game_state cur_state_;
    int state_tics_;

    int combo_size_;
    int score_;
    int score_delta_;
    int level_jukugo_left_;
    int level_score_delta_;

    falling_block falling_block_queue_[2];
    int falling_block_index_;

    int num_dropping_blocks_;

    struct dropping_block
    {
        int col_;
        int type_;
        float height_, dest_height_;
        float speed_;
        bool active_;
    } dropping_blocks_[64];

    int hint_r_, hint_c_;

    std::list<std::unique_ptr<sprite>> sprites_;

    world_event_listener *event_listener_;

    static const int NUM_NEW_KANJI_PER_LEVEL = 9;
    static const int NUM_INITIAL_FILLED_ROWS = 2;
};

void world_init();
