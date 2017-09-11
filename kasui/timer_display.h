#ifndef TIMER_DISPLAY_H_
#define TIMER_DISPLAY_H_

struct abstract_action;

namespace g2d {
class mat4;
};

class timer_display
{
public:
	timer_display();

	void reset(int tics);
	void draw(const g2d::mat4& proj_modelview, float alpha) const;
	void update(uint32_t dt);

	int get_tics_left() const
	{ return tics_left; }

	bool finished() const
	{ return tics_left == 0; }

	bool blinking() const
	{ return tics_left <= MIN_SAFE_SECS*1000; }

private:
	enum { MIN_SAFE_SECS = 10 };
	int tics_left;

	const g2d::glyph_info *digit_glyphs_[10];
	const g2d::glyph_info *byou_;
	const g2d::texture *texture_;
};

#endif // TIMER_DISPLAY_H_
