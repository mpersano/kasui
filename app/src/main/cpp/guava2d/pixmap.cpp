#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cassert>

#include <png.h>

#include "file.h"
#include "pixmap.h"
#include "panic.h"

namespace g2d {

template <class T>
static inline T min(T a, T b) { return a < b ? a : b; }

pixmap *
pixmap::load(const char *path)
{
	file_input_stream file(path);

	png_structp png_ptr;
	if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)) == nullptr)
		panic("png_create_read_struct failed");

	png_infop info_ptr;
	if ((info_ptr = png_create_info_struct(png_ptr)) == nullptr)
		panic("png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		panic("some kind of png error");

	png_init_io(png_ptr, file.get_raw_stream());
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

	int color_type = png_get_color_type(png_ptr, info_ptr);
	int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	if (bit_depth != 8)
		panic("invalid bit depth in PNG");

	type pixmap_type = INVALID;

	switch (color_type) {
		case PNG_COLOR_TYPE_GRAY:
			pixmap_type = GRAY;
			break;

		case PNG_COLOR_TYPE_GRAY_ALPHA:
			pixmap_type = GRAY_ALPHA;
			break;

		case PNG_COLOR_TYPE_RGB:
			pixmap_type = RGB;
			break;

		case PNG_COLOR_TYPE_RGBA:
			pixmap_type = RGB_ALPHA;
			break;

		default:
			panic("invalid color type in PNG");
	}

	const int width = png_get_image_width(png_ptr, info_ptr);
	const int height = png_get_image_height(png_ptr, info_ptr);

	const int stride = width*get_pixel_size(pixmap_type);

	pixmap *pm = new pixmap(width, height, pixmap_type);

	png_bytep *rows = png_get_rows(png_ptr, info_ptr);

	for (int i = 0; i < height; i++)
		memcpy(&pm->bits_[i*stride], rows[i], stride);

	png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

	return pm;
}

pixmap::pixmap(int width, int height, type pixmap_type)
: width_(width)
, height_(height)
, bits_(new uint8_t[width*height*get_pixel_size(pixmap_type)])
, type_(pixmap_type)
{
	::memset(bits_, 0, width*height*get_pixel_size());
}

pixmap::~pixmap()
{
	delete[] bits_;
}

uint8_t
pixmap::get_pixel_alpha(int row, int col) const
{
	if (row < 0 || row >= height_ || col < 0 || col >= width_)
		return 0;

	switch (type_) {
		case RGB_ALPHA:
			return (reinterpret_cast<const uint32_t *>(bits_))[row*width_ + col] >> 24;

		case GRAY_ALPHA:
			return (reinterpret_cast<const uint16_t *>(bits_))[row*width_ + col] >> 8;

		default:
			return 0xff;
	}
}

void
pixmap::resize(int new_width, int new_height)
{
	if (new_width == width_ && new_height == height_)
		return;
	
	const int pixel_size = get_pixel_size();

	uint8_t *new_bits = new uint8_t[new_width*new_height*pixel_size];
	::memset(new_bits, 0, new_width*new_height*pixel_size);

	const int copy_height = min(height_, new_height);
	const int copy_width = min(width_, new_width);

	for (int i = 0; i < copy_height; i++) {
		uint8_t *dest = &new_bits[i*new_width*pixel_size];
		uint8_t *src = &bits_[i*width_*pixel_size];
		::memcpy(dest, src, copy_width*pixel_size);
	}

	delete[] bits_;

	width_ = new_width;
	height_ = new_height;
	bits_ = new_bits;
}

void
pixmap::downsample(int scale)
{
	if (scale == 1)
		return;

	const int pixel_size = get_pixel_size();

	const int new_width = width_/scale;
	const int new_height = height_/scale;

	uint8_t *new_bits = new uint8_t[new_width*new_height*pixel_size];

	uint8_t *dest = new_bits;
	const uint8_t *src = bits_;

	int pixel_sum[pixel_size*width_];

	for (int i = 0; i < new_height; i++) {
		::memset(pixel_sum, 0, pixel_size*width_*sizeof *pixel_sum);

		for (int j = 0; j < scale; j++) {
			for (int k = 0; k < pixel_size*width_; k++)
				pixel_sum[k] += *src++;
		}

		const int *p = pixel_sum;

		for (int j = 0; j < new_width; j++) {
			for (int k = 0; k < pixel_size; k++) {
				int v = 0;

				for (int l = 0; l < scale; l++)
					v += p[k + l*pixel_size];

				*dest++ = v/scale/scale;
			}

			p += pixel_size*scale;
		}
	}

	delete[] bits_;

	width_ = new_width;
	height_ = new_height;
	bits_ = new_bits;
}

#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

void
pixmap::save(const char *path) const
{
	FILE *fp;

	if ((fp = fopen(path, "wb")) == nullptr)
		panic("fopen %s for write failed: %s", strerror(errno));

	png_structp png_ptr;

	if ((png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)nullptr, nullptr, nullptr)) == nullptr)
		panic("png_create_write_struct");

	png_infop info_ptr;

	if ((info_ptr = png_create_info_struct(png_ptr)) == nullptr)
		panic("png_create_info_struct");

	if (setjmp(png_jmpbuf(png_ptr)))
		panic("png error");

	png_init_io(png_ptr, fp);

	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

	int color_type;

	switch (type_) {
		case GRAY:
			color_type = PNG_COLOR_TYPE_GRAY;
			break;

		case GRAY_ALPHA:
			color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;

		case RGB:
			color_type = PNG_COLOR_TYPE_RGB;
			break;

		case RGB_ALPHA:
		default:
			color_type = PNG_COLOR_TYPE_RGBA;
			break;
	}

	png_set_IHDR(
		png_ptr,
		info_ptr,
		width_, height_,
		8,
		color_type,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	const int stride = width_*get_pixel_size();

	for (int i = 0; i < height_; i++)
		png_write_row(png_ptr, reinterpret_cast<png_byte *>(&bits_[i*stride]));

	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
}

int
pixmap::get_pixel_size() const
{
	return get_pixel_size(type_);
}

int
pixmap::get_pixel_size(type pixmap_type)
{
	switch (pixmap_type) {
		case GRAY:
			return 1;

		case GRAY_ALPHA:
			return 2;

		case RGB:
			return 3;

		case RGB_ALPHA:
			return 4;

		default:
			assert(0);
			return 0; // XXX
	}
}

}
