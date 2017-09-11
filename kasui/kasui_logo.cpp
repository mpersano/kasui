#include "guava2d/vertex_array.h"
#include "guava2d/sprite_manager.h"

#include "program_registry.h"
#include "common.h"
#include "action.h"
#include "tween.h"
#include "kasui_logo.h"

kasui_logo::kasui_logo()
: bg(g2d::sprite_manager::get_instance().get_sprite("logo-bg.png"))
, ka(g2d::sprite_manager::get_instance().get_sprite("logo-ka.png"))
, sui(g2d::sprite_manager::get_instance().get_sprite("logo-sui.png"))
, ka_scale(1)
, sui_scale(1)
, ka_mix(0)
, sui_mix(0)
{
	static const float max_scale = 1.15;

	action =
	  (new parallel_action_group)
	    ->add((new sequential_action_group)
		->add(new property_change_action<in_back_tween<float> >(&ka_scale, 1, max_scale, 12*MS_PER_TIC))
	  	->add(new property_change_action<out_bounce_tween<float> >(&ka_scale, max_scale, 1, 12*MS_PER_TIC))
		->add(new delay_action(80*MS_PER_TIC)))
	    ->add(new property_change_action<linear_tween<float> >(&ka_mix, 1, 0, 20*MS_PER_TIC))
	    ->add((new sequential_action_group)
		->add(new delay_action(30*MS_PER_TIC))
		->add(new property_change_action<in_back_tween<float> >(&sui_scale, 1, max_scale, 12*MS_PER_TIC))
	  	->add(new property_change_action<out_bounce_tween<float> >(&sui_scale, max_scale, 1, 12*MS_PER_TIC)))
	    ->add((new sequential_action_group)
	    	->add(new delay_action(30*MS_PER_TIC))
		->add(new property_change_action<linear_tween<float> >(&sui_mix, 1, 0, 20*MS_PER_TIC)));
}

void
kasui_logo::reset()
{
	title_widget::reset();

	ka_scale = sui_scale = 1;
	ka_mix = sui_mix = 0;
	action->reset();
}

void
kasui_logo::update(uint32_t dt)
{
	title_widget::update(dt);

	action->step(dt);

	if (action->done())
		action->reset();
}

void
kasui_logo::draw(const g2d::mat4& proj_modelview) const
{
	if (cur_state == OUTSIDE)
		return;

	static const float logo_y_from = 1., logo_y_to = 0.;
	float sy, alpha;

	switch (cur_state) {
		case ENTERING:
			{
			const float t = static_cast<float>(state_t)/ENTER_T;
			sy = out_bounce_tween<float>()(logo_y_from, logo_y_to, t);
			alpha = t;
			}
			break;

		case LEAVING:
			{
			const float t = static_cast<float>(state_t)/LEAVE_T;
			sy = in_back_tween<float>()(logo_y_to, logo_y_from, t);
			alpha = 1. - t;
			}
			break;

		case INSIDE:
			sy = logo_y_to;
			alpha = 1;
			break;

		default:
			assert(0);
	}

	const float w = bg->get_width();
	const float h = bg->get_height();

	const float x = .5*(window_width - w);
	const float y = window_height - h + sy*h;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	program_texture_uniform_alpha& prog = get_program_instance<program_texture_uniform_alpha>();
	prog.use();
	prog.set_texture(0);
	prog.set_alpha(alpha);

	g2d::mat4 mv = proj_modelview*g2d::mat4::translation(x, y, 0);

	// background

	prog.set_proj_modelview_matrix(mv);
	bg->draw(0, 0);

	// ka

	prog.set_proj_modelview_matrix(mv*g2d::mat4::scale(1, ka_scale, 1));
	ka->draw(0, 0);

	// sui

	prog.set_proj_modelview_matrix(mv*g2d::mat4::scale(1, sui_scale, 1));
	sui->draw(0, 0);
}
