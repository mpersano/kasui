#pragma once

#include "theme.h"
#include "guava2d/rgb.h"

struct gradient
{
    gradient(g2d::rgb *from, g2d::rgb *to)
        : from(from)
        , to(to)
    {
    }

    g2d::rgb *from, *to;
};

struct color_scheme
{
    g2d::rgb *main_color;
    g2d::rgb *alternate_color;
    gradient *background_gradient;
    gradient *text_gradient;
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
    color_scheme *color_schemes[NUM_THEMES];
};

extern settings cur_settings;

void load_settings();
