#include <string>
#include <vector>

#include "panic.h"
#include "file.h"
#include "shader.h"

namespace g2d {

shader::shader(GLenum type)
: id_(glCreateShader(type))
{ }

shader::~shader()
{
	glDeleteShader(id_);
}

void
shader::set_source(const char *source) const
{
	glShaderSource(id_, 1, &source, nullptr);
}

void
shader::load_source(const char *path) const
{
	file_input_stream file(path);

	size_t size = file.size();

	char *source = new char[size + 1];
	file.read(reinterpret_cast<uint8_t *>(source), size);
	source[size] = '\0';

	set_source(source);

	delete[] source;
}

void
shader::compile() const
{
	glCompileShader(id_);

	GLint status;
	glGetShaderiv(id_, GL_COMPILE_STATUS, &status);
	if (!status)
		panic("%s", get_info_log().c_str());
}

std::string
shader::get_info_log() const
{
	std::string log_string;

	GLint length;
	glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &length);

	if (length > 0) {
		GLint written;

		std::vector<GLchar> data(length + 1);
		glGetShaderInfoLog(id_, length, &written, &data[0]);

		log_string.assign(data.begin(), data.begin() + written);
	}

	return log_string;
}

}
