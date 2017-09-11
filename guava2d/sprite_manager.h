#ifndef SPRITE_MANAGER_H_
#define SPRITE_MANAGER_H_

#include <map>
#include <string>

#include "sprite.h"

namespace g2d {

class sprite_manager
{
	typedef std::map<std::string, sprite *> dict_type;
	typedef typename dict_type::value_type dict_value_type;

public:
	static sprite_manager& get_instance();

	sprite *get_sprite(const char *name);

	void add_sprite_sheet(const char *path);

private:
	sprite_manager();

	dict_type sprite_dict_;
};

}

#endif // SPRITE_MANAGER_H_
