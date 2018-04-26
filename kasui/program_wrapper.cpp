#include "guava2d/program.h"
#include "guava2d/rgb.h"
#include "guava2d/vec2.h"
#include "guava2d/vec3.h"

#include "program_wrapper.h"

program_wrapper::program_wrapper(const char *vert_shader_path, const char *frag_shader_path)
    : program_(new g2d::program)
    , vert_shader_path_(vert_shader_path)
    , frag_shader_path_(frag_shader_path)
{
    load_and_compile();
}

void program_wrapper::load_and_compile()
{
    program_->initialize();

    g2d::shader vert_shader(GL_VERTEX_SHADER);
    vert_shader.load_source(vert_shader_path_.c_str());
    vert_shader.compile();

    g2d::shader frag_shader(GL_FRAGMENT_SHADER);
    frag_shader.load_source(frag_shader_path_.c_str());
    frag_shader.compile();

    program_->attach(vert_shader);
    program_->attach(frag_shader);
}

void program_wrapper::reload()
{
    load_and_compile();
    initialize();
}

void program_wrapper::initialize()
{
    bind_attribute_locations();

    program_->link();

    proj_modelview_matrix_location_ = program_->get_uniform_location("proj_modelview_matrix");

    initialize_uniform_locations();
}

program_wrapper::~program_wrapper()
{
    delete program_;
}

void program_wrapper::set_proj_modelview_matrix(const g2d::mat4 &projection) const
{
    g2d::program::set_uniform(proj_modelview_matrix_location_, projection);
}

void program_wrapper::set_proj_modelview_matrix(const GLfloat *projection) const
{
    g2d::program::set_uniform_matrix4(proj_modelview_matrix_location_, projection);
}

void program_wrapper::use()
{
    program_->use();
}

program_flat::program_flat()
    : program_wrapper("shaders/vert_flat.glsl", "shaders/frag_flat.glsl")
{
}

void program_flat::bind_attribute_locations()
{
    program_->bind_attrib_location(0, "position");
}

void program_flat::initialize_uniform_locations()
{
    color_location_ = program_->get_uniform_location("color");
}

void program_flat::set_color(const g2d::rgba &color) const
{
    g2d::program::set_uniform(color_location_, color);
}

program_color::program_color()
    : program_wrapper("shaders/vert_color.glsl", "shaders/frag_color.glsl")
{
}

void program_color::bind_attribute_locations()
{
    program_->bind_attrib_location(0, "position");
    program_->bind_attrib_location(1, "color");
}

void program_color::initialize_uniform_locations()
{
}

program_texture_base::program_texture_base(const char *vert_shader_path, const char *frag_shader_path)
    : program_wrapper(vert_shader_path, frag_shader_path)
{
}

void program_texture_base::bind_attribute_locations()
{
    program_->bind_attrib_location(0, "position");
    program_->bind_attrib_location(1, "texcoord");
}

void program_texture_base::initialize_uniform_locations()
{
    texture_location_ = program_->get_uniform_location("texture");
}

void program_texture_base::set_texture(int texunit) const
{
    g2d::program::set_uniform_i(texture_location_, texunit);
}

program_texture_decal::program_texture_decal()
    : program_texture_base("shaders/vert_texture_decal.glsl", "shaders/frag_texture_decal.glsl")
{
}

program_texture_color::program_texture_color()
    : program_texture_base("shaders/vert_texture_color.glsl", "shaders/frag_texture_color.glsl")
{
}

void program_texture_color::bind_attribute_locations()
{
    program_texture_base::bind_attribute_locations();
    program_->bind_attrib_location(2, "color");
}

program_3d_texture_color::program_3d_texture_color()
    : program_texture_base("shaders/vert_3d_texture_color.glsl", "shaders/frag_texture_color.glsl")
{
}

void program_3d_texture_color::bind_attribute_locations()
{
    program_texture_base::bind_attribute_locations();
    program_->bind_attrib_location(2, "color");
}

program_texture_alpha::program_texture_alpha()
    : program_texture_base("shaders/vert_texture_alpha.glsl", "shaders/frag_texture_alpha.glsl")
{
}

void program_texture_alpha::bind_attribute_locations()
{
    program_texture_base::bind_attribute_locations();
    program_->bind_attrib_location(2, "alpha");
}

program_texture_uniform_alpha::program_texture_uniform_alpha()
    : program_texture_base("shaders/vert_texture_decal.glsl", "shaders/frag_texture_uniform_alpha.glsl")
{
}

void program_texture_uniform_alpha::initialize_uniform_locations()
{
    program_texture_base::initialize_uniform_locations();
    alpha_location_ = program_->get_uniform_location("alpha");
}

void program_texture_uniform_alpha::set_alpha(float alpha) const
{
    g2d::program::set_uniform_f(alpha_location_, alpha);
}

program_texture_uniform_color::program_texture_uniform_color()
    : program_texture_base("shaders/vert_texture_decal.glsl", "shaders/frag_texture_uniform_color.glsl")
{
}

void program_texture_uniform_color::initialize_uniform_locations()
{
    program_texture_base::initialize_uniform_locations();
    color_location_ = program_->get_uniform_location("color");
}

void program_texture_uniform_color::set_color(const g2d::rgba &color) const
{
    g2d::program::set_uniform(color_location_, color);
}

program_text_base::program_text_base(const char *vert_shader_path, const char *frag_shader_path)
    : program_texture_base(vert_shader_path, frag_shader_path)
{
}

void program_text_base::initialize_uniform_locations()
{
    program_texture_base::initialize_uniform_locations();
    color_location_ = program_->get_uniform_location("color");
}

void program_text_base::set_color(const g2d::rgba &color) const
{
    g2d::program::set_uniform(color_location_, color);
}

program_text::program_text()
    : program_text_base("shaders/vert_texture_decal.glsl", "shaders/frag_text.glsl")
{
}

program_text_outline::program_text_outline()
    : program_text_base("shaders/vert_texture_decal.glsl", "shaders/frag_text_outline.glsl")
{
}

program_text_alpha_base::program_text_alpha_base(const char *vert_shader_path, const char *frag_shader_path)
    : program_texture_base(vert_shader_path, frag_shader_path)
{
}

void program_text_alpha_base::bind_attribute_locations()
{
    program_texture_base::bind_attribute_locations();
    program_->bind_attrib_location(2, "alpha");
}

void program_text_alpha_base::initialize_uniform_locations()
{
    program_texture_base::initialize_uniform_locations();
    color_location_ = program_->get_uniform_location("color");
}

void program_text_alpha_base::set_color(const g2d::rgb &color) const
{
    g2d::program::set_uniform(color_location_, color);
}

program_text_alpha::program_text_alpha()
    : program_text_alpha_base("shaders/vert_texture_alpha.glsl", "shaders/frag_text_alpha.glsl")
{
}

program_text_outline_alpha::program_text_outline_alpha()
    : program_text_alpha_base("shaders/vert_texture_alpha.glsl", "shaders/frag_text_outline_alpha.glsl")
{
}

program_text_gradient_base::program_text_gradient_base(const char *vert_shader_path, const char *frag_shader_path)
    : program_texture_base(vert_shader_path, frag_shader_path)
{
}

void program_text_gradient_base::bind_attribute_locations()
{
    program_texture_base::bind_attribute_locations();
    program_->bind_attrib_location(2, "mix_factor");
}

void program_text_gradient_base::initialize_uniform_locations()
{
    program_texture_base::initialize_uniform_locations();
    top_color_location_ = program_->get_uniform_location("top_color");
    bottom_color_location_ = program_->get_uniform_location("bottom_color");
}

void program_text_gradient_base::set_top_color(const g2d::rgba &color) const
{
    g2d::program::set_uniform(top_color_location_, color);
}

void program_text_gradient_base::set_bottom_color(const g2d::rgba &color) const
{
    g2d::program::set_uniform(bottom_color_location_, color);
}

program_text_gradient::program_text_gradient()
    : program_text_gradient_base("shaders/vert_texture_gradient.glsl", "shaders/frag_text_gradient.glsl")
{
}

program_text_outline_gradient::program_text_outline_gradient()
    : program_text_gradient_base("shaders/vert_texture_gradient.glsl", "shaders/frag_text_outline_gradient.glsl")
{
}

program_intro_text::program_intro_text()
    : program_texture_base("shaders/vert_texture_gradient.glsl", "shaders/frag_intro_text.glsl")
{
}

void program_intro_text::bind_attribute_locations()
{
    program_texture_base::bind_attribute_locations();
    program_->bind_attrib_location(2, "mix_factor");
}

void program_intro_text::initialize_uniform_locations()
{
    program_texture_base::initialize_uniform_locations();
    top_color_text_location_ = program_->get_uniform_location("top_color_text");
    bottom_color_text_location_ = program_->get_uniform_location("bottom_color_text");
    top_color_outline_location_ = program_->get_uniform_location("top_color_outline");
    bottom_color_outline_location_ = program_->get_uniform_location("bottom_color_outline");
}

void program_intro_text::set_top_color_text(const g2d::rgba &color) const
{
    g2d::program::set_uniform(top_color_text_location_, color);
}

void program_intro_text::set_bottom_color_text(const g2d::rgba &color) const
{
    g2d::program::set_uniform(bottom_color_text_location_, color);
}

void program_intro_text::set_top_color_outline(const g2d::rgba &color) const
{
    g2d::program::set_uniform(top_color_outline_location_, color);
}

void program_intro_text::set_bottom_color_outline(const g2d::rgba &color) const
{
    g2d::program::set_uniform(bottom_color_outline_location_, color);
}

program_timer_text::program_timer_text()
    : program_texture_base("shaders/vert_texture_decal.glsl", "shaders/frag_timer_text.glsl")
{
}

void program_timer_text::initialize_uniform_locations()
{
    program_texture_base::initialize_uniform_locations();
    text_color_location_ = program_->get_uniform_location("text_color");
    outline_color_location_ = program_->get_uniform_location("outline_color");
}

void program_timer_text::set_text_color(const g2d::rgba &color) const
{
    g2d::program::set_uniform(text_color_location_, color);
}

void program_timer_text::set_outline_color(const g2d::rgba &color) const
{
    g2d::program::set_uniform(outline_color_location_, color);
}
