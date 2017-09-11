#include "texture.h"
#include "vertex_array.h"
#include "sprite.h"

namespace g2d {

sprite::sprite(const g2d::texture *texture, int left, int top, int width, int height, int left_margin, int right_margin, int top_margin, int bottom_margin)
: left_(left)
, top_(top)
, width_(width)
, height_(height)
, left_margin_(left_margin)
, right_margin_(right_margin)
, top_margin_(top_margin)
, bottom_margin_(bottom_margin)
, texture_(texture)
{
	const int texture_width = texture->get_texture_width();
	const int texture_height = texture->get_texture_height();

	int s = 2; // texture_->get_downsample_scale();

	const float du = static_cast<float>(width)/s/texture_width;
	const float dv = static_cast<float>(height)/s/texture_height;

	u0_ = static_cast<float>(left)/s/texture_width;
	u1_ = u0_ + du;

	v0_ = static_cast<float>(top)/s/texture_height;
	v1_ = v0_ + dv;
}

void
sprite::draw(float x, float y) const
{
	texture_->bind();

	x += left_margin_;
	y += bottom_margin_;

	static g2d::vertex_array_texuv va(4);

	va.reset();
	va << x, y, u0_, v1_;
	va << x + width_, y, u1_, v1_;
	va << x, y + height_, u0_, v0_;
	va << x + width_, y + height_, u1_, v0_;

	va.draw(GL_TRIANGLE_STRIP);
}

}
