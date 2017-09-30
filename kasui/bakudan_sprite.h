#ifndef BAKUDAN_CLIP_H_
#define BAKUDAN_CLIP_H_

#include "world.h"

class bakudan_sprite : public sprite
{
public:
	bakudan_sprite(float x, float y);
	~bakudan_sprite();

	bool update(uint32_t dt);
	void draw() const;

protected:
	float x_center, y_center;
	float start_angle;
	int tics;
	const g2d::texture *glow_texture;
};

#endif // BAKUDAN_CLIP_H_
