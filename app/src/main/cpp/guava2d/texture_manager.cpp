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
	const texture *load(const std::string& source);
	void put(const std::string& name, texture *t);

	void load_all();

private:
    std::unordered_map<std::string, texture *> texture_dict_;
} g_texture_manager;

const texture *
texture_manager::load(const std::string& source)
{
	auto it = texture_dict_.find(source);

	if (it == texture_dict_.end()) {
		printf("loading %s...\n", source.c_str());
		it = texture_dict_.insert(it, {source, new texture(pixmap::load(source.c_str()))});
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
