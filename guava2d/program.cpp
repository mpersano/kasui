#include <cstdio>
#include <vector>

#include "panic.h"
#include "program.h"

namespace g2d {

program::program()
: id_(0)
{
}

program::~program()
{
	if (id_)
		glDeleteProgram(id_);
}

void
program::initialize()
{
	id_ = glCreateProgram();
}

void
program::attach(const shader& shader) const
{
	glAttachShader(id_, shader.get_id());
}

void
program::link() const
{
	glLinkProgram(id_);
}

GLint
program::get_uniform_location(const GLchar *name) const
{
	GLint rv = glGetUniformLocation(id_, name);
	if (rv == -1)
		panic("get_uniform_location failed for %s\n", name);
	return rv;
}

GLint
program::get_attrib_location(const GLchar *name) const
{
	GLint rv = glGetAttribLocation(id_, name);
	if (rv == -1)
		fprintf(stderr, "get_attribute_location failed for %s\n", name);
	return rv;
}

void
program::bind_attrib_location(GLuint index, const GLchar *name) const
{
	glBindAttribLocation(id_, index, name);
}

void
program::set_uniform_f(const GLchar *name, GLfloat v0) const
{
	set_uniform_f(get_uniform_location(name), v0);
}

void
program::set_uniform_f(const GLchar *name, GLfloat v0, GLfloat v1) const
{
	set_uniform_f(get_uniform_location(name), v0, v1);
}

void
program::set_uniform_f(const GLchar *name, GLfloat v0, GLfloat v1, GLfloat v2) const
{
	set_uniform_f(get_uniform_location(name), v0, v1, v2);
}

void
program::set_uniform_f(const GLchar *name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const
{
	set_uniform_f(get_uniform_location(name), v0, v1, v2, v3);
}

void
program::set_uniform_i(const GLchar *name, GLint v0) const
{
	set_uniform_i(get_uniform_location(name), v0);
}

void
program::set_uniform_i(const GLchar *name, GLint v0, GLint v1) const
{
	set_uniform_i(get_uniform_location(name), v0, v1);
}

void
program::set_uniform_i(const GLchar *name, GLint v0, GLint v1, GLint v2) const
{
	set_uniform_i(get_uniform_location(name), v0, v1, v2);
}

void
program::set_uniform_i(const GLchar *name, GLint v0, GLint v1, GLint v2, GLint v3) const
{
	set_uniform_i(get_uniform_location(name), v0, v1, v2, v3);
}

void
program::set_uniform(const GLchar *name, const vec2& v) const
{
	set_uniform(get_uniform_location(name), v);
}

void
program::set_uniform(const GLchar *name, const vec3& v) const
{
	set_uniform(get_uniform_location(name), v);
}

void
program::set_uniform(const GLchar *name, const mat4& mat) const
{
	set_uniform(get_uniform_location(name), mat);
}

void
program::set_uniform(const GLchar *name, const rgb& color) const
{
	set_uniform(get_uniform_location(name), color);
}

void
program::set_uniform(const GLchar *name, const rgba& color) const
{
	set_uniform(get_uniform_location(name), color);
}

void
program::set_uniform_matrix4(const GLchar *name, const GLfloat *matrix) const
{
	set_uniform_matrix4(get_uniform_location(name), matrix);
}

void
program::set_uniform_f(GLint location, GLfloat v0)
{
	glUniform1f(location, v0);
}

void
program::set_uniform_f(GLint location, GLfloat v0, GLfloat v1)
{
	glUniform2f(location, v0, v1);
}

void
program::set_uniform_f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
	glUniform3f(location, v0, v1, v2);
}

void
program::set_uniform_f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	glUniform4f(location, v0, v1, v2, v3);
}

void
program::set_uniform_i(GLint location, GLint v0)
{
	glUniform1i(location, v0);
}

void
program::set_uniform_i(GLint location, GLint v0, GLint v1)
{
	glUniform2i(location, v0, v1);
}

void
program::set_uniform_i(GLint location, GLint v0, GLint v1, GLint v2)
{
	glUniform3i(location, v0, v1, v2);
}

void
program::set_uniform_i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
	glUniform4i(location, v0, v1, v2, v3);
}

void
program::set_uniform(GLint location, const vec2& v)
{
	glUniform2f(location, v.x, v.y);
}

void
program::set_uniform(GLint location, const vec3& v)
{
	glUniform3f(location, v.x, v.y, v.z);
}

void
program::set_uniform(GLint location, const mat4& mat)
{
	GLfloat gl_matrix[16] = {
		mat.m11, mat.m21, mat.m31, 0,
		mat.m12, mat.m22, mat.m32, 0,
		mat.m13, mat.m23, mat.m33, 0,
		mat.m14, mat.m24, mat.m34, 1 };

	set_uniform_matrix4(location, gl_matrix);
}

void
program::set_uniform_matrix4(GLint location, const GLfloat *matrix)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
}

void
program::set_uniform(GLint location, const rgb& color)
{
	glUniform3f(location, color.r, color.g, color.b);
}

void
program::set_uniform(GLint location, const rgba& color)
{
	glUniform4f(location, color.r, color.g, color.b, color.a);
}

void
program::use() const
{
	glUseProgram(id_);
}

std::string
program::get_info_log() const
{
	std::string log_string;

	GLint length;
	glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &length);

	if (length) {
		GLint written;

		std::vector<GLchar> data(length + 1);
		glGetProgramInfoLog(id_, length, &written, &data[0]);

		log_string.assign(data.begin(), data.begin() + written);
	}

	return log_string;
}

}
