#pragma once

#include <guava2d/vec2.h>

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

class sakura_fubuki
{
public:
	sakura_fubuki();

	void reset();
	void draw() const;
	void update(uint32_t dt);

private:
	static constexpr int NUM_PETALS = 20;
	sakura_petal petals_[NUM_PETALS];

	const g2d::texture *petal_texture_;
};
