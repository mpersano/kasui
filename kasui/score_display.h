#pragma once

namespace g2d {
class glyph_info;
};

class score_display
{
public:
	score_display();

	void reset();
	void enqueue_value(int value);
	void set_value(int value);

	void draw(float x_base, float y_base) const;
	void update(uint32_t dt);

private:
	enum { NUM_DIGITS = 6, };
	struct digit {
		int value;
		int bump;
	} digits[NUM_DIGITS];

	enum { MAX_QUEUE_SIZE = 16 };
	struct {
		int tics;
		int value;
	} queue[MAX_QUEUE_SIZE];
	int queue_head, queue_tail, queue_size;

	const g2d::glyph_info *digit_glyphs_[10];
	const g2d::texture *texture_;
};
