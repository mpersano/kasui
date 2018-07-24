#pragma once

#include <string>

#include "texture.h"

namespace g2d {

const texture *load_texture(const std::string& source);
void put_texture(const std::string& name, texture *t);
void reload_all_textures();

}
