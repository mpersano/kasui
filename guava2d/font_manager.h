#pragma once

#include "font.h"

#include <string>

namespace g2d {

class font;

const font *load_font(const std::string& source);

}
