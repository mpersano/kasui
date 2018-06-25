#include "sprite_manager.h"

#include "guava2d/file.h"
#include "guava2d/texture_manager.h"
#include "noncopyable.h"
#include "panic.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <unordered_map>

namespace g2d {

namespace {

class sprite_manager : private noncopyable
{
public:
    const sprite *get_sprite(const std::string &name);
    void load_sprite_sheet(const std::string &path);

private:
    std::unordered_map<std::string, sprite *> sprite_dict_;
} g_sprite_manager;

const sprite *sprite_manager::get_sprite(const std::string &name)
{
    auto it = sprite_dict_.find(name);

    if (it == sprite_dict_.end())
        panic("failed to locate sprite `%s'", name.c_str());

    return it->second;
}

void sprite_manager::load_sprite_sheet(const std::string &source)
{
    const auto path = source + ".spr";;
    file_input_stream file(path.c_str());

    const int num_sprites = file.read_uint16();
    (void)file.read_uint8(); // # of sheets

    for (int i = 0; i < num_sprites; i++) {
        const std::string sprite_name = file.read_string();
        const int left_margin = file.read_uint16();
        const int right_margin = file.read_uint16();
        const int top_margin = file.read_uint16();
        const int bottom_margin = file.read_uint16();

        const int sheet_index = file.read_uint8();
        const int left = file.read_uint16();
        const int top = file.read_uint16();
        const int width = file.read_uint16();
        const int height = file.read_uint16();

        std::stringstream ss;
        ss << source << '.' << std::setfill('0') << std::setw(3) << sheet_index << ".png";
        const auto texture_path = ss.str();

        const auto texture = load_texture(texture_path.c_str());

        auto sp = new sprite(texture, left, top, width, height, left_margin,
                             right_margin, top_margin, bottom_margin);
        sprite_dict_.insert({sprite_name, sp});
    }
}
}

void load_sprite_sheet(const std::string &path)
{
    g_sprite_manager.load_sprite_sheet(path);
}

const sprite *get_sprite(const std::string &name)
{
    g_sprite_manager.get_sprite(name);
}
}
