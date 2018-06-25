#include "texture_manager.h"

#include "pixmap.h"

#include <cstdio>
#include <algorithm>
#include <unordered_map>

namespace g2d {

namespace {

class texture_manager
{
public:
	void set_downsample_scale(int scale);

	const texture *load(const std::string& source);
	void put(const std::string& name, texture *t);

	void load_all();

private:
    std::unordered_map<std::string, texture *> texture_dict_;
	int downsample_scale_ = 1;
} g_texture_manager;

void
texture_manager::set_downsample_scale(int scale)
{
	downsample_scale_ = scale;
}

const texture *
texture_manager::load(const std::string& source)
{
	auto it = texture_dict_.find(source);

	if (it == texture_dict_.end()) {
		printf("loading %s...\n", source.c_str());
		it = texture_dict_.insert(it, {source, new texture(pixmap::load(source.c_str()), downsample_scale_)});
	}

	return it->second;
}

void
texture_manager::put(const std::string& name, texture *t)
{
	texture_dict_.insert({name, t});
}

void
texture_manager::load_all()
{
    for (auto& v : texture_dict_)
        v.second->load();
}

}

void set_texture_downsample_scale(int scale)
{
    g_texture_manager.set_downsample_scale(scale);
}

const texture *load_texture(const std::string& source)
{
    return g_texture_manager.load(source);
}

void put_texture(const std::string& name, texture *t)
{
    g_texture_manager.put(name, t);
}

void reload_all_textures()
{
    g_texture_manager.load_all();
}

}
