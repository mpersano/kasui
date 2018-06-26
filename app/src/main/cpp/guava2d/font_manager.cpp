#include "font_manager.h"

#include <unordered_map>

namespace g2d {

namespace {

class font_manager
{
public:
	const font *load(const std::string& source);

private:
    std::unordered_map<std::string, font *> font_dict_;
} g_font_manager;

const font *font_manager::load(const std::string& source)
{
	auto it = font_dict_.find(source);

	if (it == font_dict_.end())
		it = font_dict_.insert(it, {source, new font(source.c_str())});

	return it->second;
}

}

const font *load_font(const std::string& source)
{
    return g_font_manager.load(source);
}

}
