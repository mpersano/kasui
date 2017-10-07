#include "guava2d/g2dgl.h"
#include "guava2d/sprite_manager.h"

#include "render.h"
#include "common.h"
#include "program_registry.h"
#include "pause_button.h"

pause_button::pause_button(float x, float y, void (*on_activation_fn)(void *), void *extra)
: x_base(x)
, y_base(y)
, on_activation_fn(on_activation_fn)
, extra(extra)
, sprite_unselected_(g2d::sprite_manager::get_instance().get_sprite("pause-0.png"))
, sprite_selected_(g2d::sprite_manager::get_instance().get_sprite("pause-1.png"))
{
	reset();
}

void
pause_button::reset()
{
	is_selected = false;
}

void
pause_button::draw(float alpha) const
{
	render::set_blend_mode(blend_mode::ALPHA_BLEND);

	render::push_matrix();
	render::translate(x_base, y_base);

	render::set_color({ 1.f, 1.f, 1.f, alpha*(is_selected ? 1.f : .5f)});
	render::draw_sprite(is_selected ? sprite_selected_ : sprite_unselected_, 0, 0, 10);

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
		on_activation_fn(extra);
		is_selected = false;
		return true;
	}
}
