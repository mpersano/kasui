#include <cstdio>

#include "font_manager.h"

namespace g2d {

font_manager::font_manager()
{ }

font_manager &
font_manager::get_instance()
{
	static font_manager fm;
	return fm;
}

const font *
font_manager::load(const char *source)
{
	auto it = font_dict_.find(source);

	if (it == font_dict_.end())
		it = font_dict_.insert(it, dict_value_type(source, new font(source)));

	return it->second;
}

}
