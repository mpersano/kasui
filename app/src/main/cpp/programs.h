#pragma once

#include <string>

namespace g2d
{
class program;
}

enum class program
{
    flat,
    sprite_2d,
    sprite_3d,
    text_inner,
    text_outline,
    text_gradient,
    grid_background,
    program_count,
};

void initialize_programs();
const g2d::program *get_program(program p);
