#include <unordered_map>

#include "panic.h"
#include "noncopyable.h"
#include "guava2d/file.h"
#include "guava2d/texture_manager.h"

#include "sprite_manager.h"

namespace g2d {

namespace {

class sprite_manager : private noncopyable
{
public:
	const sprite *get_sprite(const std::string& name);

	void load_sprite_sheet(const std::string& path);

private:
	std::unordered_map<std::string, sprite *> sprite_dict_;
} g_sprite_manager;

const sprite *
sprite_manager::get_sprite(const std::string& name)
{
	auto it = sprite_dict_.find(name);

	if (it == sprite_dict_.end())
		panic("failed to locate sprite `%s'", name);

	return it->second;
}

void
sprite_manager::load_sprite_sheet(const std::string& source)
{
	char path[512];
	sprintf(path, "%s.spr", source.c_str());

	file_input_stream file(path);

	int num_sprites = file.read_uint16();
	(void)file.read_uint8(); // # of sheets

	for (int i = 0; i < num_sprites; i++) {
		std::string sprite_name = file.read_string();
		int left_margin = file.read_uint16();
		int right_margin = file.read_uint16();
		int top_margin = file.read_uint16();
		int bottom_margin = file.read_uint16();

		int sheet_index = file.read_uint8();
		int left = file.read_uint16();
		int top = file.read_uint16();
		int width = file.read_uint16();
		int height = file.read_uint16();

		char texture_path[80];
		sprintf(texture_path, "%s.%03d.png", source.c_str(), sheet_index);

		auto sp = new sprite(texture_manager::get_instance().load(texture_path), left, top, width, height, left_margin, right_margin, top_margin, bottom_margin);
		sprite_dict_.insert({ sprite_name, sp });
	}
}

}

void load_sprite_sheet(const std::string& path)
{
	g_sprite_manager.load_sprite_sheet(path);
}

const sprite *get_sprite(const std::string& name)
{
	g_sprite_manager.get_sprite(name);
}

}
