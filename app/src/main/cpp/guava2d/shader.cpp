#include <string>
#include <vector>

#include "panic.h"
#include "file.h"
#include "shader.h"

namespace g2d {

shader::shader(GLenum type)
: id_(GL_CHECK_R(glCreateShader(type)))
{ }

shader::~shader()
{
	GL_CHECK(glDeleteShader(id_));
}

void
shader::set_source(const char *source) const
{
	GL_CHECK(glShaderSource(id_, 1, &source, nullptr));
}

void
shader::load_source(const char *path) const
{
	file_input_stream file(path);

	const auto size = file.size();

	std::vector<char> source(size + 1, 0);
	file.read(reinterpret_cast<uint8_t *>(&source[0]), size);

	set_source(&source[0]);
}

void
shader::compile() const
{
	GL_CHECK(glCompileShader(id_));

	GLint status;
	GL_CHECK(glGetShaderiv(id_, GL_COMPILE_STATUS, &status));
	if (!status)
		panic("%s", get_info_log().c_str());
}

std::string
shader::get_info_log() const
{
	std::string log_string;

	GLint length;
	GL_CHECK(glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &length));

	if (length > 0) {
		GLint written;

		std::vector<GLchar> data(length + 1);
		GL_CHECK(glGetShaderInfoLog(id_, length, &written, &data[0]));

		log_string.assign(data.begin(), data.begin() + written);
	}

	return log_string;
}

}
