#include "guava2d/g2dgl.h"
#include "guava2d/sprite_manager.h"

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
pause_button::draw(const g2d::mat4& mat, float alpha) const
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	program_texture_uniform_alpha& prog = get_program_instance<program_texture_uniform_alpha>();
	prog.use();
	prog.set_proj_modelview_matrix(mat*g2d::mat4::translation(x_base, y_base, 0));
	prog.set_texture(0);
	prog.set_alpha(alpha*(is_selected ? 1. : .5));

	(is_selected ? sprite_selected_ : sprite_unselected_)->draw(0, 0);
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
