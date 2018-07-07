#pragma once

#include <string>

#include "g2dgl.h"
#include "shader.h"
#include "rgb.h"
#include "vec2.h"
#include "vec3.h"

namespace g2d {

class program
{
public:
	program();
	~program();

	program(const program&) = delete;
	program& operator=(const program&) = delete;

	void initialize();
	void attach(const shader& shader) const;
	void link() const;

	void bind_attrib_location(GLuint index, const GLchar *name) const;
	GLint get_attrib_location(const GLchar *name) const;

	GLint get_uniform_location(const GLchar *name) const;

	void set_uniform_f(const GLchar *name, GLfloat v0) const;
	void set_uniform_f(const GLchar *name, GLfloat v0, GLfloat v1) const;
	void set_uniform_f(const GLchar *name, GLfloat v0, GLfloat v1, GLfloat v2) const;
	void set_uniform_f(const GLchar *name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const;

	void set_uniform_i(const GLchar *name, GLint v0) const;
	void set_uniform_i(const GLchar *name, GLint v0, GLint v1) const;
	void set_uniform_i(const GLchar *name, GLint v0, GLint v1, GLint v2) const;
	void set_uniform_i(const GLchar *name, GLint v0, GLint v1, GLint v2, GLint v3) const;

	void set_uniform(const GLchar *name, const vec2& v) const;
	void set_uniform(const GLchar *name, const vec3& v) const;
	void set_uniform(const GLchar *name, const mat4& mat) const;
	void set_uniform(const GLchar *name, const rgb& color) const;
	void set_uniform(const GLchar *name, const rgba& color) const;

	void set_uniform_matrix4(const GLchar *name, const GLfloat *matrix) const;

	static void set_uniform_f(GLint location, GLfloat v0);
	static void set_uniform_f(GLint location, GLfloat v0, GLfloat v1);
	static void set_uniform_f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	static void set_uniform_f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

	static void set_uniform_i(GLint location, GLint v0);
	static void set_uniform_i(GLint location, GLint v0, GLint v1);
	static void set_uniform_i(GLint location, GLint v0, GLint v1, GLint v2);
	static void set_uniform_i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

	static void set_uniform(GLint location, const vec2& v);
	static void set_uniform(GLint location, const vec3& v);
	static void set_uniform(GLint location, const mat4& mat);
	static void set_uniform(GLint location, const rgb& color);
	static void set_uniform(GLint location, const rgba& color);

	static void set_uniform_matrix4(GLint location, const GLfloat *matrix);

	void use() const;

	std::string get_info_log() const;

private:
	GLuint id_;
};

}
