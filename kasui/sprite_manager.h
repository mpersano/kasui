#pragma once

#include <unordered_map>
#include <string>

#include "sprite.h"

namespace g2d {

class sprite_manager
{
public:
	static sprite_manager& get_instance();

	sprite *get_sprite(const char *name);

	void add_sprite_sheet(const char *path);

private:
	sprite_manager();

	std::unordered_map<std::string, sprite *> sprite_dict_;
};

}
