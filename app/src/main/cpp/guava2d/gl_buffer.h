#pragma once

#include "g2dgl.h"

namespace g2d {

class gl_buffer
{
public:
	gl_buffer(GLenum target);
	~gl_buffer();

    gl_buffer(const gl_buffer&) = delete;
    gl_buffer& operator=(const gl_buffer&) = delete;

	void bind() const;
	void unbind() const;

	void buffer_data(GLsizei size, const void *data, GLenum usage) const;

	void *map_range(GLintptr offset, GLsizei length, GLbitfield access) const;
	void unmap() const;

private:
	GLenum target_;
	GLuint id_;
};

};
