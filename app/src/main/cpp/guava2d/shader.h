#pragma once

#include <string>

#include "g2dgl.h"

namespace g2d {

class shader
{
public:
	// type: GL_FRAGMENT_SHADER or GL_VERTEX_SHADER
	shader(GLenum type);
	~shader();

	shader(const shader&) = delete;
	shader& operator=(const shader&) = delete;

	void set_source(const char *source) const;
	void load_source(const char *path) const;

	void compile() const;

	const GLuint get_id() const
	{ return id_; }

	std::string get_info_log() const;

private:
	GLuint id_;
};

}
