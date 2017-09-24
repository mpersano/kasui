#ifndef SAKURA_H_
#define SAKURA_H_

#include "guava2d/vec2.h"
#include "guava2d/vertex_array.h"

struct sakura_petal
{
	float size;
	g2d::vec2 pos, dir;
	float angle, delta_angle;
	int tics, ttl;
	float phase_0, phi_0, radius_0;
	float phase_1, phi_1, radius_1;
	float alpha;

	void reset(bool);
	void draw(const g2d::texture *texture) const;
	void update(uint32_t dt);
};

namespace g2d {
class texture;
}

struct sakura_fubuki
{
	sakura_fubuki();

	void reset();
	void draw() const;
	void update(uint32_t dt);

	enum { NUM_PETALS = 20 };
	sakura_petal petals[NUM_PETALS];

	const g2d::texture *petal_texture;
};

#endif // SAKURA_H_
