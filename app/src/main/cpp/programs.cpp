#include "programs.h"

#include "noncopyable.h"

#include <guava2d/program.h>

#include <vector>

namespace
{
class program_manager : private noncopyable
{
public:
    void initialize();
    const g2d::program *get(program p) const;

private:
    void load_programs();

    std::vector<const g2d::program *> programs_;
} g_program_manager;

const g2d::program *program_manager::get(program p) const
{
    return programs_[static_cast<int>(p)];
}

void program_manager::initialize()
{
    load_programs();
}

void program_manager::load_programs()
{
    static const struct program_source
    {
        const char *vertex_shader;
        const char *fragment_shader;
    } program_sources[static_cast<int>(program::program_count)] =
    {
        { "shaders/flat.vert", "shaders/flat.frag" },
        { "shaders/sprite.vert", "shaders/sprite.frag" },
        { "shaders/sprite_3d.vert", "shaders/sprite.frag" },
        { "shaders/sprite.vert", "shaders/text_inner.frag" },
        { "shaders/sprite.vert", "shaders/text_outline.frag" },
        { "shaders/sprite_2c.vert", "shaders/text_gradient.frag" },
        { "shaders/grid_background.vert", "shaders/sprite.frag" },
    };

    programs_.reserve(static_cast<int>(program::program_count));

    for (const auto &source : program_sources)
    {
        g2d::shader vert_shader(GL_VERTEX_SHADER);
        vert_shader.load_source(source.vertex_shader);
        vert_shader.compile();

        g2d::shader frag_shader(GL_FRAGMENT_SHADER);
        frag_shader.load_source(source.fragment_shader);
        frag_shader.compile();

        auto program = new g2d::program;

        program->initialize();
        program->attach(vert_shader);
        program->attach(frag_shader);
        program->link();

        programs_.push_back(program);
    }
}
}

void initialize_programs()
{
    g_program_manager.initialize();
}

const g2d::program *get_program(program p)
{
    return g_program_manager.get(p);
}
