#ifndef PIXMAP_H_
#define PIXMAP_H_

#include <cstdint>
#include <vector>

namespace g2d {

class pixmap {
public:
	enum type { GRAY, GRAY_ALPHA, RGB, RGB_ALPHA, INVALID };

	pixmap(int width, int height, type pixmap_type);

	pixmap(const pixmap&) = delete;
	pixmap& operator=(const pixmap&) = delete;

	int get_width() const
	{ return width_; }

	int get_height() const
	{ return height_; }

	const uint8_t *get_bits() const
	{ return &bits_[0]; }

	uint8_t *get_bits()
	{ return &bits_[0]; }

	type get_type() const
	{ return type_; }

	int get_pixel_size() const;
	static int get_pixel_size(type pixmap_type);

	void resize(int new_width, int new_height);

	uint8_t get_pixel_alpha(int row, int col) const;

	static pixmap *load(const char *path);

	void save(const char *path) const;

protected:
	int width_;
	int height_;
    std::vector<uint8_t> bits_;
	type type_;
};

}

#endif // PIXMAP_H_
