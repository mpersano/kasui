#ifndef PROGRAM_WRAPPERS_H_
#define PROGRAM_WRAPPERS_H_

#include <string>

#include "guava2d/g2dgl.h"

namespace g2d {
class mat4;
class program;
class rgb;
class rgba;
class vec2;
};

class program_wrapper
{
public:
    program_wrapper(const char *vert_shader_path, const char *frag_shader_path);
    virtual ~program_wrapper();

    void initialize();

    void set_proj_modelview_matrix(const g2d::mat4 &proj_modelview) const;
    void set_proj_modelview_matrix(const GLfloat *proj_modelview) const;

    void use();

    void reload();

    const g2d::program *get_raw() const { return program_; }

protected:
    void load_and_compile();

    virtual void bind_attribute_locations() = 0;
    virtual void initialize_uniform_locations() = 0;

    GLint proj_modelview_matrix_location_;

    g2d::program *program_;
    std::string vert_shader_path_;
    std::string frag_shader_path_;
};

class program_flat : public program_wrapper
{
public:
    program_flat();

    void set_color(const g2d::rgba &color) const;

protected:
    void bind_attribute_locations();
    void initialize_uniform_locations();

    GLint color_location_;
};

class program_color : public program_wrapper
{
public:
    program_color();

protected:
    void bind_attribute_locations();
    void initialize_uniform_locations();
};

class program_texture_base : public program_wrapper
{
public:
    program_texture_base(const char *vert_shader_path, const char *frag_shader_path);

    void set_texture(int texunit) const;

protected:
    void bind_attribute_locations();
    void initialize_uniform_locations();

    GLint texture_location_;
};

class program_texture_decal : public program_texture_base
{
public:
    program_texture_decal();
};

class program_texture_color : public program_texture_base
{
public:
    program_texture_color();

protected:
    void bind_attribute_locations();
};

class program_3d_texture_color : public program_texture_base
{
public:
    program_3d_texture_color();

protected:
    void bind_attribute_locations();
};

class program_texture_alpha : public program_texture_base
{
public:
    program_texture_alpha();

protected:
    void bind_attribute_locations();
};

class program_texture_uniform_alpha : public program_texture_base
{
public:
    program_texture_uniform_alpha();

    void set_alpha(float alpha) const;

protected:
    void initialize_uniform_locations();

    GLint alpha_location_;
};

class program_texture_uniform_color : public program_texture_base
{
public:
    program_texture_uniform_color();

    void set_color(const g2d::rgba &color) const;

protected:
    void initialize_uniform_locations();

    GLint color_location_;
};

class program_text_base : public program_texture_base
{
public:
    program_text_base(const char *vert_shader_path, const char *frag_shader_path);

    void set_color(const g2d::rgba &color) const;

protected:
    void initialize_uniform_locations();

    GLint color_location_;
};

class program_text : public program_text_base
{
public:
    program_text();
};

class program_text_outline : public program_text_base
{
public:
    program_text_outline();
};

class program_text_alpha_base : public program_texture_base
{
public:
    program_text_alpha_base(const char *vert_shader_path, const char *frag_shader_path);

    void set_color(const g2d::rgb &color) const;

protected:
    void bind_attribute_locations();
    void initialize_uniform_locations();

    GLint color_location_;
};

class program_text_alpha : public program_text_alpha_base
{
public:
    program_text_alpha();
};

class program_text_outline_alpha : public program_text_alpha_base
{
public:
    program_text_outline_alpha();
};

class program_text_gradient_base : public program_texture_base
{
public:
    program_text_gradient_base(const char *vert_shader_path, const char *frag_shader_path);

    void set_top_color(const g2d::rgba &color) const;
    void set_bottom_color(const g2d::rgba &color) const;

protected:
    void bind_attribute_locations();
    void initialize_uniform_locations();

    GLint top_color_location_;
    GLint bottom_color_location_;
};

class program_text_gradient : public program_text_gradient_base
{
public:
    program_text_gradient();
};

class program_text_outline_gradient : public program_text_gradient_base
{
public:
    program_text_outline_gradient();
};

class program_intro_text : public program_texture_base
{
public:
    program_intro_text();

    void set_top_color_text(const g2d::rgba &color) const;
    void set_bottom_color_text(const g2d::rgba &color) const;

    void set_top_color_outline(const g2d::rgba &color) const;
    void set_bottom_color_outline(const g2d::rgba &color) const;

protected:
    void bind_attribute_locations();
    void initialize_uniform_locations();

    GLint top_color_text_location_;
    GLint bottom_color_text_location_;
    GLint top_color_outline_location_;
    GLint bottom_color_outline_location_;
};

class program_timer_text : public program_texture_base
{
public:
    program_timer_text();

    void set_text_color(const g2d::rgba &color) const;
    void set_outline_color(const g2d::rgba &color) const;

protected:
    void initialize_uniform_locations();

    GLint text_color_location_;
    GLint outline_color_location_;
};

#endif // PROGRAM_WRAPPERS_H_
