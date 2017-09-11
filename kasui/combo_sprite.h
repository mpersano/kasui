#ifndef COMBO_CLIP_H_
#define COMBO_CLIP_H_

#include <guava2d/draw_queue.h>

#include "world.h"

class combo_sprite : public sprite
{
public:
	combo_sprite(int combo_size, float x, float y, const gradient *g);

	bool update(uint32_t dt);
	void draw(const g2d::mat4& proj_modelview) const;

private:
	g2d::draw_queue text_;

	float x_origin, y_origin;
	float y_offset;
	const gradient *gradient_;

	int ttl;
};

#endif // COMBO_CLIP_H_
