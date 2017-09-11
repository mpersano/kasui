#ifndef TITLE_BACKGROUND_H_
#define TITLE_BACKGROUND_H_

#include "guava2d/vertex_array.h"

#include "kasui_logo.h"
#include "sakura.h"
#include "title_widget.h"

struct title_background
{
	title_background();
	virtual ~title_background();

	void reset();
	void update(uint32_t dt);
	void draw(const g2d::mat4& proj_modelview) const;

	void initialize_bg_billboard();

	title_widget *fg_billboard;
	title_widget *logo;

	const g2d::texture *bg_texture;
	g2d::vertex_array_texuv bg_billboard;

	sakura_fubuki sakura;
};

#endif // TITLE_BACKGROUND_H_
