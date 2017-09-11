#pragma once

#include <map>

#include <guava2d/vertex_array.h>

namespace g2d {
class mat4;
class texture;
};

class text_box
{
public:
	text_box(float width);

	void set_text(const wchar_t *text);

	void draw(const g2d::mat4& mat, float alpha) const;

	float get_height() const
	{ return height_; }

	static const int HEIGHT = 200;

private:
	static const int BORDER_RADIUS = 16;
	static const int LINE_HEIGHT = 46;

	void initialize_frame();
	void draw_frame(const g2d::mat4& mat, float alpha) const;

	float width_, height_;
	const g2d::texture *frame_texture_;

	typedef g2d::indexed_vertex_array<
			GLushort,
			g2d::vertex::attrib<GLfloat, 2>,
			g2d::vertex::attrib<GLfloat, 2> > vertex_array;

	typedef std::map<const g2d::texture *, vertex_array> map_type;
	typedef map_type::value_type map_value_type;

	map_type quads_;

	g2d::vertex_array_texuv frame_va_[3];
};
