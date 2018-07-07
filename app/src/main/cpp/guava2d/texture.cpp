#include <cstdio>

#include "panic.h"
#include "pixmap.h"
#include "texture.h"

namespace g2d {

template <typename T>
static T
next_power_of_2(T n)
{
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

texture::texture(pixmap *pm, int downsample_scale)
: pixmap_(pm)
, downsample_scale_(downsample_scale)
, orig_pixmap_width_(pixmap_->get_width())
, orig_pixmap_height_(pixmap_->get_height())
, texture_id_(0)
{
	pixmap_->downsample(downsample_scale);

	pixmap_width_ = pixmap_->get_width();
	pixmap_height_ = pixmap_->get_height();

	texture_width_ = next_power_of_2(pixmap_width_);
	texture_height_ = next_power_of_2(pixmap_height_);

	pixmap_->resize(texture_width_, texture_height_);

	load();
}

texture::~texture()
{
	GL_CHECK(glDeleteTextures(1, &texture_id_));
}

void
texture::bind() const
{
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_id_));
}

void
texture::load()
{
	GL_CHECK(glGenTextures(1, &texture_id_));

	bind();

	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

	upload_pixmap();
}

void
texture::upload_pixmap() const
{
	bind();

	GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

	/* format == internalFormat in OpenGLES 1.1, see http://www.khronos.org/opengles/sdk/1.1/docs/man/glTexImage2D.xml */
	GLint format = 0;

	switch (pixmap_->get_type()) {
		case pixmap::GRAY:
			format = GL_LUMINANCE;
			break;

		case pixmap::GRAY_ALPHA:
			format = GL_LUMINANCE_ALPHA;
			break;

		case pixmap::RGB:
			format = GL_RGB;
			break;

		case pixmap::RGB_ALPHA:
			format = GL_RGBA;
			break;

		default:
			panic("invalid pixmap format");
			break;
	}


	GL_CHECK(glTexImage2D(
		GL_TEXTURE_2D,
		0,
		format,
		texture_width_, texture_height_,
		0,
		format,
		GL_UNSIGNED_BYTE,
		pixmap_->get_bits()));
}

}
