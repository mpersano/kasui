#include "guava2d/g2dgl.h"

#include "sprite_manager.h"
#include "render.h"
#include "common.h"
#include "program_registry.h"
#include "pause_button.h"

pause_button::pause_button(float x, float y)
: x_base(x)
, y_base(y)
, sprite_unselected_(g2d::get_sprite("pause-0.png"))
, sprite_selected_(g2d::get_sprite("pause-1.png"))
{
	reset();
}

void
pause_button::reset()
{
	is_selected = false;
}

void
pause_button::set_callback(std::function<void(void)> callback)
{
	callback_ = callback;
}

void
pause_button::draw(float alpha) const
{
	render::set_blend_mode(blend_mode::ALPHA_BLEND);

	render::push_matrix();
	render::translate(x_base, y_base);

	render::set_color({ 1.f, 1.f, 1.f, alpha*(is_selected ? 1.f : .5f)});
	(is_selected ? sprite_selected_ : sprite_unselected_)->draw(0, 0, 10);

	render::pop_matrix();
}

bool
pause_button::on_touch_down(float x, float y)
{
	return is_selected = x >= x_base && x < x_base + SIZE && y >= y_base && y < y_base + SIZE;
}

bool
pause_button::on_touch_up()
{
	if (!is_selected) {
		return false;
	} else {
		if (callback_)
			callback_();
		is_selected = false;
		return true;
	}
}
