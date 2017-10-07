#include "guava2d/texture.h"
#include "render.h"
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
sprite::draw(float x, float y, int layer) const
{
    const auto x0 = x + left_margin_;
    const auto x1 = x0 + width_;

    const auto y0 = y + bottom_margin_;
    const auto y1 = y0 + height_;

    const auto u0 = u0_;
    const auto u1 = u1_;

    const auto v0 = v0_;
    const auto v1 = v1_;

    render::draw_quad(
            texture_,
            { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } },
            { { u0, v1 }, { u1, v1 }, { u1, v0 }, { u0, v0 } },
            layer);
}

}
