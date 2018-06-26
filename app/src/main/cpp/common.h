#ifndef COMMON_H_
#define COMMON_H_

#include "guava2d/vec3.h"

enum
{
    FPS = 30 // update rate
};

static const int MS_PER_TIC = (1000 / FPS);

enum
{
    NUM_LEVELS = 9
};

extern int viewport_width, viewport_height;
extern float window_width, window_height;

extern int cur_level, cur_score;
extern bool practice_mode;

extern const char *options_file_path;
extern const char *leaderboard_file_path;
extern const char *jukugo_hits_file_path;

struct options;
extern options *cur_options;

extern unsigned dpad_state;

enum
{
    DPAD_UP = 1,
    DPAD_DOWN = 2,
    DPAD_LEFT = 4,
    DPAD_RIGHT = 8,
};

void quit();

class state;

state *get_cur_state();

state *get_prev_state();

void push_state(state *);

void pop_state();

void start_in_game(int level);

void continue_in_game();

void start_main_menu();

void start_in_game_menu();

void start_stats_page();

void start_tutorial();

void start_hiscore_list();

void start_hiscore_input();

void start_credits();

g2d::mat4 get_ortho_projection();

float frand();

template <typename T>
T frand(const T &from, const T &to)
{
    return from + frand() * (to - from);
}

int irand(int from, int to);

void draw_fade_overlay(float alpha);

#endif // COMMON_H_
