#pragma once

#include <map>

#include "action.h"
#include "world.h"

struct jukugo;
struct gradient;

namespace g2d {
class font;
class texture;
};

class jukugo_info_sprite : public sprite
{
public:
	jukugo_info_sprite(const jukugo *jukugo_info, float x, float y, const gradient *g);
	~jukugo_info_sprite();

	bool update(uint32_t dt);
	void draw() const;

private:
	void initialize_quads(const jukugo *jukugo_info);

	void initialize_string_quads(
		const g2d::font *font,
		float x, float y,
		float shade_top, float shade_bottom,
		const wchar_t *str);

	void draw_quads() const;

	typedef g2d::indexed_vertex_array<
			GLushort,
			g2d::vertex::attrib<GLfloat, 2>,
			g2d::vertex::attrib<GLfloat, 2>,
			g2d::vertex::attrib<GLfloat, 1> > vertex_array;

	typedef std::map<const g2d::texture *, vertex_array> map_type;
	typedef map_type::value_type map_value_type;

	map_type quads_;

	float x_base, y_base;

	float flip, alpha, z;
	abstract_action *action;
	const gradient *gradient_;
};
