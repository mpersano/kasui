#include "panic.h"
#include "guava2d/file.h"
#include "guava2d/texture_manager.h"

#include "sprite_manager.h"

namespace g2d {

sprite_manager::sprite_manager()
{ }

sprite_manager &
sprite_manager::get_instance()
{
	static sprite_manager sm;
	return sm;
}

sprite *
sprite_manager::get_sprite(const char *name)
{
	auto it = sprite_dict_.find(name);

	if (it == sprite_dict_.end())
		panic("failed to locate sprite `%s'", name);

	return it->second;
}

void
sprite_manager::add_sprite_sheet(const char *source)
{
	char path[512];
	sprintf(path, "%s.spr", source);

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
		sprintf(texture_path, "%s.%03d.png", source, sheet_index);

		sprite *sp = new sprite(texture_manager::get_instance().load(texture_path), left, top, width, height, left_margin, right_margin, top_margin, bottom_margin);
		sprite_dict_.insert({ sprite_name, sp });
	}
}

}
