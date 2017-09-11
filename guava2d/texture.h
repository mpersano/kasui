#ifndef TEXTURE_H_
#define TEXTURE_H_

#include "g2dgl.h"
#include "pixmap.h"

namespace g2d {

class texture {
public:
	texture(pixmap *pm, int downsample_scale);

	virtual ~texture();

	const pixmap *get_pixmap() const
	{ return pixmap_; }

	pixmap *get_pixmap()
	{ return pixmap_; }

	int get_texture_width() const
	{ return texture_width_; }

	int get_texture_height() const
	{ return texture_height_; }

	int get_pixmap_width() const
	{ return orig_pixmap_width_; }

	int get_pixmap_height() const
	{ return orig_pixmap_height_; }

	int get_downsample_scale() const
	{ return downsample_scale_; }

	float get_u_scale() const
	{ return static_cast<float>(pixmap_width_)/texture_width_; }

	float get_v_scale() const
	{ return static_cast<float>(pixmap_height_)/texture_height_; }

	void bind() const;

	void load();

	void upload_pixmap() const;

private:
	pixmap *pixmap_;

	int downsample_scale_;

	int orig_pixmap_width_, orig_pixmap_height_;
	int pixmap_width_, pixmap_height_;
	int texture_width_, texture_height_;

	GLuint texture_id_;
};

}

#endif // TEXTURE_H_
