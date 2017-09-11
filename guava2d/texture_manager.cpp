#include <cstdio>
#include <algorithm>

#include "pixmap.h"
#include "texture_manager.h"

namespace g2d {

texture_manager::texture_manager()
: downsample_scale_(1)
{ }

texture_manager &
texture_manager::get_instance()
{
	static texture_manager tm;
	return tm;
}

void
texture_manager::set_downsample_scale(int scale)
{
	downsample_scale_ = scale;
}

const texture *
texture_manager::load(const char *source)
{
	auto it = texture_dict_.find(source);

	if (it == texture_dict_.end()) {
		printf("loading %s...\n", source);
		it = texture_dict_.insert(it, dict_value_type(source, new texture(pixmap::load(source), downsample_scale_)));
	}

	return it->second;
}

void
texture_manager::put(const char *name, texture *t)
{
	texture_dict_.insert(dict_value_type(name, t));
}

void
texture_manager::load_all()
{
	std::for_each(
		texture_dict_.begin(),
		texture_dict_.end(),
		[](dict_value_type& v) { v.second->load(); });
}

}
