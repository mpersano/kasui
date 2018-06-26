#pragma once

#include <string>

#include <guava2d/program.h>

const g2d::program *load_program(const std::string &vert_shader, const std::string &frag_shader);
