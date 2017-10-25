#pragma once

#include <string>

#include "sprite.h"

namespace g2d {

void load_sprite_sheet(const std::string& path);
const sprite *get_sprite(const std::string& name);

}
