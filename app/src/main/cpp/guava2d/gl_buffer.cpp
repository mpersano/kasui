#include "gl_buffer.h"

namespace g2d {

gl_buffer::gl_buffer(GLenum target)
: target_{target}
{
	GL_CHECK(glGenBuffers(1, &id_));
}

gl_buffer::~gl_buffer()
{
	GL_CHECK(glDeleteBuffers(1, &id_));
}

void
gl_buffer::bind() const
{
	GL_CHECK(glBindBuffer(target_, id_));
}

void
gl_buffer::unbind() const
{
	GL_CHECK(glBindBuffer(target_, 0));
}

void
gl_buffer::buffer_data(GLsizei size, const void *data, GLenum usage) const
{
	GL_CHECK(glBufferData(target_, size, data, usage));
}

void *
gl_buffer::map_range(GLintptr offset, GLsizei length, GLbitfield access) const
{
	return GL_CHECK_R(glMapBufferRange(target_, offset, length, access));
}

void
gl_buffer::unmap() const
{
	GL_CHECK(glUnmapBuffer(target_));
}

}
