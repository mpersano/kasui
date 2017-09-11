#include <cstdio>
#include <cassert>

#include "guava2d/g2dgl.h"
#include "guava2d/font_manager.h"
#include "guava2d/texture_manager.h"
#include "guava2d/sprite_manager.h"
#include "guava2d/draw_queue.h"
#include "guava2d/rgb.h"

#include "common.h"
#include "options.h"
#include "program_registry.h"
#include "menu.h"
#include "sounds.h"
#include "in_game.h"
#include "main_menu.h"
#include "in_game.h"
#include "in_game_menu.h"

namespace {

class vertical_menu : public menu
{
private:
	enum {
		ITEM_WIDTH = 280,
		ITEM_HEIGHT = 80,
	};

	g2d::vec2 get_item_position(int item_index) const;
};

g2d::vec2
vertical_menu::get_item_position(int item_index) const
{
	const float total_height = num_items*ITEM_HEIGHT;

	const menu_item *item = get_item_at(item_index);

	const float base_y = .5*(window_height + total_height) - .5*ITEM_HEIGHT;
	const float base_x = .5*(window_width - ITEM_WIDTH);

	float y = base_y - item_index*ITEM_HEIGHT - .5*ITEM_HEIGHT;

	float offset_scale = 1. - get_cur_alpha();
	float x_offset = (.5*window_width + item_index*72.)*offset_scale*offset_scale;

	float x = base_x + x_offset - 32.*sinf(item->get_active_t()*M_PI);

	return g2d::vec2(x, y);
}

} // namespace

class in_game_menu_impl
{
public:
	in_game_menu_impl();

	void reset();
	void redraw() const;
	void update(uint32_t dt);
	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_touch_move(float x, float y);
	void on_back_key();
	void on_menu_key();

private:
	void create_root_menu();
	void create_options_menu();

	void set_cur_menu(menu *p);

	enum state {
		STATE_FADE_IN,
		STATE_ACTIVE,
		STATE_FADE_OUT,
		STATE_FADE_TO_JUKUGO_STATS,
	};
	void set_cur_state(state s);

	void draw_frame(const g2d::mat4& proj_modelview, float alpha) const;

	vertical_menu root_menu_, options_menu_;
	menu *cur_menu_;
	bool touch_is_down_;
	
	enum {
		FRAME_HEIGHT = 340,
		FADE_IN_TICS = 10*MS_PER_TIC,
	};

	state cur_state_;
	int state_tics_;

	g2d::vertex_array_color frame_va_;
};

in_game_menu_impl::in_game_menu_impl()
{
	create_root_menu();
	create_options_menu();

	reset();
}

void
in_game_menu_impl::create_root_menu()
{
	g2d::sprite_manager& sm = g2d::sprite_manager::get_instance();

	root_menu_.append_action_item(
		SOUND_MENU_VALIDATE,
		sm.get_sprite("resume-0.png"),
		sm.get_sprite("resume-1.png"),
		[this] { set_cur_state(STATE_FADE_OUT); },
		[] { pop_state(); },
		true);

	root_menu_.append_action_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("options-0.png"),
		sm.get_sprite("options-1.png"),
		[] { },
		[this] { set_cur_menu(&options_menu_); });

	root_menu_.append_action_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("stats-0.png"),
		sm.get_sprite("stats-1.png"),
		[this] { set_cur_state(STATE_FADE_TO_JUKUGO_STATS); },
		[] {
			// i'm going to regret this some day i know it
			extern ::state *get_main_menu_state();
			static_cast<main_menu_state *>(get_main_menu_state())->unfade();
			start_stats_page();
		});

	root_menu_.append_action_item(
		SOUND_MENU_BACK,
		sm.get_sprite("quit-0.png"),
		sm.get_sprite("quit-1.png"),
		[this] { set_cur_state(STATE_FADE_OUT); },
		[] {
			pop_state();
			pop_state();
			get_cur_state()->reset();
		},
		false);
}

void
in_game_menu_impl::create_options_menu()
{
	g2d::sprite_manager& sm = g2d::sprite_manager::get_instance();

	options_menu_.append_toggle_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("hints-on-0.png"), sm.get_sprite("hints-on-1.png"),
		sm.get_sprite("hints-off-0.png"), sm.get_sprite("hints-off-1.png"),
		&cur_options->enable_hints,
		[] (int enable_hints) {
			extern ::state *get_in_game_state();
			static_cast<in_game_state *>(get_in_game_state())->set_enable_hints(enable_hints);
		});

	options_menu_.append_toggle_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("sound-on-0.png"), sm.get_sprite("sound-on-1.png"),
		sm.get_sprite("sound-off-0.png"), sm.get_sprite("sound-off-1.png"),
		&cur_options->enable_sound,
		[] (int enable_sound) {
			if (!enable_sound)
				stop_all_sounds();
			else
				start_sound(SOUND_MENU_SELECT, false);
		});

	options_menu_.append_action_item(
		SOUND_MENU_BACK,
		sm.get_sprite("back-0.png"), sm.get_sprite("back-1.png"),
		[] { },
		[this] { set_cur_menu(&root_menu_); },
		true);
}

void
in_game_menu_impl::reset()
{
	touch_is_down_ = false;

	set_cur_menu(&root_menu_);
	set_cur_state(STATE_FADE_IN);
}

void
in_game_menu_impl::set_cur_state(state s)
{
	cur_state_ = s;
	state_tics_ = 0;
}

void
in_game_menu_impl::set_cur_menu(menu *p)
{
	cur_menu_ = p;
	cur_menu_->reset();
}

void
in_game_menu_impl::draw_frame(const g2d::mat4& proj_modelview, float alpha) const
{
	alpha *= 1.5;
	if (alpha > 1)
		alpha = 1;

	const int a = alpha*192;

	const float y0 = .5*(window_height - FRAME_HEIGHT);
	const float y1 = .5*(window_height + FRAME_HEIGHT);

	static g2d::vertex_array_color gv;
	gv.reset();

	// top/bottom

	// bottom (black)

	gv << 0, 0, 0, 0, 0, a;
	gv << window_width, 0, 0, 0, 0, a;
	gv << window_width, y0, 0, 0, 0, a;

	gv << window_width, y0, 0, 0, 0, a;
	gv << 0, y0, 0, 0, 0, a;
	gv << 0, 0, 0, 0, 0, a;

	// top (black)

	gv << 0, y1, 0, 0, 0, a;
	gv << window_width, y1, 0, 0, 0, a;
	gv << window_width, window_height, 0, 0, 0, a;

	gv << window_width, window_height, 0, 0, 0, a;
	gv << 0, window_height, 0, 0, 0, a;
	gv << 0, y1, 0, 0, 0, a;

	// center (white)

#if 0
	enum { NUM_SEGMENTS = 32, };

	const float dy = (y1 - y0)/NUM_SEGMENTS;
	float y = y0;

	for (int i = 0; i < NUM_SEGMENTS; i++) {
		const int r = 190;
		const int g = 190;
		const int b = 190;

		// const int a = alpha*(i%2 == 0 ? 64 : 80);
		const int a = alpha*(i%2 == 0 ? 192 : 222);

		gv << 0, y, r, g, b, a;
		gv << window_width, y, r, g, b, a;
		gv << window_width, y + dy, r, g, b, a;

		gv << window_width, y + dy, r, g, b, a;
		gv << 0, y + dy, r, g, b, a;
		gv << 0, y, r, g, b, a;

		y += dy;
	}
#else
	const int r = 190;
	const int g = 190;
	const int b = 190;

	gv << 0, y0, r, g, b, a;
	gv << window_width, y0, r, g, b, a;
	gv << window_width, y1, r, g, b, a;

	gv << window_width, y1, r, g, b, a;
	gv << 0, y1, r, g, b, a;
	gv << 0, y0, r, g, b, a;
#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	program_color& prog = get_program_instance<program_color>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);

	gv.draw(GL_TRIANGLES);
}

void
in_game_menu_impl::redraw() const
{
	get_prev_state()->redraw();

	const g2d::mat4 ortho = get_ortho_projection();

	float alpha;

	switch (cur_state_) {
		case STATE_FADE_IN:
			alpha = static_cast<float>(state_tics_)/FADE_IN_TICS;
			break;

		case STATE_FADE_OUT:
			alpha = cur_menu_->get_cur_alpha();
			break;

		default:
			alpha = 1;
			break;
	}

	draw_frame(ortho, alpha);

	cur_menu_->draw(ortho);

	if (cur_state_ == STATE_FADE_TO_JUKUGO_STATS)
		draw_fade_overlay(1. - cur_menu_->get_cur_alpha());
}

void
in_game_menu_impl::update(uint32_t dt)
{
	if (cur_state_ == STATE_FADE_IN) {
		if ((state_tics_ += dt) >= FADE_IN_TICS)
			set_cur_state(STATE_ACTIVE);
	} else {
		cur_menu_->update(dt);
	}
}

void
in_game_menu_impl::on_touch_down(float x, float y)
{
	touch_is_down_ = true;
	cur_menu_->on_touch_down(x, y);
}

void
in_game_menu_impl::on_touch_up()
{
	touch_is_down_ = false;
	cur_menu_->on_touch_up();
}

void
in_game_menu_impl::on_touch_move(float x, float y)
{
	if (touch_is_down_)
		cur_menu_->on_touch_down(x, y);
}

void
in_game_menu_impl::on_back_key()
{
	cur_menu_->on_back_key();
}

void
in_game_menu_impl::on_menu_key()
{
}

in_game_menu_state::in_game_menu_state()
: impl_(new in_game_menu_impl)
{ }

in_game_menu_state::~in_game_menu_state()
{
	delete impl_;
}

void
in_game_menu_state::reset()
{
	impl_->reset();
}

void
in_game_menu_state::redraw() const
{
	impl_->redraw();
}

void
in_game_menu_state::update(uint32_t dt)
{
	impl_->update(dt);
}

void
in_game_menu_state::on_touch_down(float x, float y)
{
	impl_->on_touch_down(x, y);
}

void
in_game_menu_state::on_touch_up()
{
	impl_->on_touch_up();

}

void
in_game_menu_state::on_touch_move(float x, float y)
{
	impl_->on_touch_move(x, y);
}

void
in_game_menu_state::on_back_key()
{
	impl_->on_back_key();

}

void
in_game_menu_state::on_menu_key()
{
	impl_->on_menu_key();
}
