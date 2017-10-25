#include "noncopyable.h"
#include "program_manager.h"

#include <map>
#include <utility>

namespace {

class program_manager : private noncopyable
{
public:
    const g2d::program *get(const std::string& vert_source, const std::string& frag_source);

private:
    const g2d::program *load(const std::string& vert_source, const std::string& frag_source);

    std::map<std::pair<std::string, std::string>, const g2d::program *> cache_;
} g_program_manager;

const g2d::program *program_manager::get(const std::string& vert_source, const std::string& frag_source)
{
    const auto key = std::make_pair(vert_source, frag_source);

    auto it = cache_.find(key);
    if (it == std::end(cache_))
        it = cache_.insert(it, { key, load(vert_source, frag_source) });

    return it->second;
}

const g2d::program *program_manager::load(const std::string& vert_source, const std::string& frag_source)
{
    g2d::shader vert_shader(GL_VERTEX_SHADER);
    vert_shader.load_source(vert_source.c_str());
    vert_shader.compile();

    g2d::shader frag_shader(GL_FRAGMENT_SHADER);
    frag_shader.load_source(frag_source.c_str());
    frag_shader.compile();

    auto program = new g2d::program;

    program->initialize();
    program->attach(vert_shader);
    program->attach(frag_shader);
    program->link();

    return program;
}

}

const g2d::program *load_program(const std::string& vert_source, const std::string& frag_source)
{
    return g_program_manager.get(vert_source, frag_source);
}
