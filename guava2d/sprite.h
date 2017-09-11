#ifndef SPRITE_H_
#define SPRITE_H_

namespace g2d {

class texture;

class sprite
{
public:
	sprite(const g2d::texture *texture, int left, int top, int width, int height, int left_margin, int right_margin, int top_margin, int bottom_margin);

	void draw(float x, float y) const;

	const g2d::texture *get_texture() const
	{ return texture_; }

	int get_left() const
	{ return left_; }

	int get_top() const
	{ return top_; }

	int get_width() const
	{ return width_ + left_margin_ + right_margin_; }

	int get_height() const
	{ return height_ + top_margin_ + bottom_margin_; }

private:
	int left_, top_;
	int width_, height_;
	int left_margin_, right_margin_;
	int top_margin_, bottom_margin_;
	float u0_, u1_, v0_, v1_;
	const g2d::texture *texture_;
};

}

#endif // SPRITE_H_
