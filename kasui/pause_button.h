#ifndef PAUSE_BUTTON_H_
#define PAUSE_BUTTON_H_

namespace g2d {
class sprite;
class mat4;
}

class pause_button
{
public:
	pause_button(float x, float y, void (*on_activation_fn)(void *), void *extra);

	void reset();
	void draw(const g2d::mat4& mat, float alpha) const;

	bool on_touch_down(float x, float y);
	bool on_touch_up();

	static const int SIZE = 80;

private:
	float x_base, y_base;

	void (*on_activation_fn)(void *);
	void *extra;

	const g2d::sprite *sprite_unselected_;
	const g2d::sprite *sprite_selected_;

	bool is_selected;
};

#endif // PAUSE_BUTTON_H_
