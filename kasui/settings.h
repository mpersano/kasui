#pragma once

#include "theme.h"
#include "guava2d/rgb.h"

#include <array>

struct gradient
{
    g2d::rgb from, to;
};

struct color_scheme
{
    g2d::rgb main_color;
    g2d::rgb alternate_color;
    gradient background_gradient;
    gradient text_gradient;
};

struct game_settings
{
    int level_secs;
    int tics_to_drop;
    int bakudan_period;
    int hint_period;
};

struct animation_settings
{
    int drop_tics;
    int solve_tics;
    int swap_tics;
    int move_tics;
};

struct settings
{
    game_settings game;
    animation_settings animation;
    std::array<color_scheme, NUM_THEMES> color_schemes;
};

extern settings cur_settings;

void load_settings();
