#include <cstdio>
#include <cassert>

#include <stdlib.h>

#include "guava2d/g2dgl.h"
#include "guava2d/rgb.h"
#include "guava2d/texture_manager.h"

#include "sprite_manager.h"
#include "render.h"

#include "program_registry.h"
#include "options.h"
#include "common.h"
#include "tween.h"
#include "menu.h"
#include "action.h"
#include "sounds.h"
#include "title_background.h"
#include "credits.h"
#include "main_menu.h"

static bool touch_is_down;

extern void on_rate_me_clicked();

enum {
	NUM_BUTTON_COLS = 3,
	NUM_BUTTON_ROWS = (NUM_LEVELS + NUM_BUTTON_COLS - 1)/NUM_BUTTON_COLS,

	FADE_IN_T = 10*MS_PER_TIC,
};

class vertical_menu : public menu
{
private:
	enum {
		ITEM_WIDTH = 280,
		ITEM_HEIGHT = 80,
	};

	g2d::vec2 get_item_position(int item_index) const;
};

class level_selection_menu : public menu
{
public:
	level_selection_menu(main_menu_impl *parent);

private:
	enum {
		ITEM_WIDTH = 200,
		ITEM_HEIGHT = 200,

		BACK_BUTTON_WIDTH = 180,
		BACK_BUTTON_HEIGHT = 80,
	};

	g2d::vec2 get_item_position(int item_index) const;

	main_menu_impl *parent;
};

class main_menu_impl
{
public:
	main_menu_impl();
	~main_menu_impl();

	void reset();
	void redraw() const;
	void update(uint32_t dt);
	void on_touch_down(float x, float y);
	void on_touch_up();
	void on_touch_move(float x, float y);
	void on_back_key();
	void on_menu_key();

	void show_root_menu();
	void show_more_menu();
	void show_game_mode_menu();
	void show_background();
	void hide_background();
	void fade_in();

	void set_state_outro();
	void set_state_active();

private:
	void create_main_menu();
	void create_more_menu();
	void create_options_menu();
	void create_game_mode_menu();

	void set_cur_menu(menu *p);

	title_background background;

	vertical_menu main_menu;
	vertical_menu options_menu;
	vertical_menu more_menu;
	vertical_menu game_mode_menu;
	level_selection_menu level_menu;

	menu *cur_menu;

	enum menu_state {
		STATE_FADE_IN,
		STATE_ACTIVE,
		STATE_OUTRO,
	};

	menu_state cur_state;
	uint32_t state_t;
};

main_menu_impl::main_menu_impl()
: level_menu(this)
{
	create_main_menu();
	create_options_menu();
	create_more_menu();
	create_game_mode_menu();
}

main_menu_impl::~main_menu_impl()
{ }

void
main_menu_impl::set_cur_menu(menu *p)
{
	cur_menu = p;
	cur_menu->reset();
}

void
main_menu_impl::show_root_menu()
{
	set_cur_menu(&main_menu);
}

void
main_menu_impl::show_background()
{
	background.fg_billboard->show();
	background.logo->show();
}

void
main_menu_impl::hide_background()
{
	background.fg_billboard->hide();
	background.logo->hide();
}

void
main_menu_impl::show_game_mode_menu()
{
	set_cur_menu(&game_mode_menu);
}

void
main_menu_impl::show_more_menu()
{
	set_cur_menu(&more_menu);
}

void
main_menu_impl::set_state_outro()
{
	cur_state = STATE_OUTRO;
}

void
main_menu_impl::set_state_active()
{
	cur_state = STATE_ACTIVE;
}

void
main_menu_impl::fade_in()
{
	cur_state = STATE_FADE_IN;
	state_t = 0;
}

g2d::vec2
vertical_menu::get_item_position(int item_index) const
{
	const float total_height = item_list.size()*ITEM_HEIGHT;

	const auto& item = item_list[item_index];

	const float base_y = .5*(window_height + total_height) - .5*ITEM_HEIGHT;
	const float base_x = window_width - ITEM_WIDTH;

	float y = base_y - item_index*ITEM_HEIGHT - .5*ITEM_HEIGHT;

	float offset_scale = 1. - get_cur_alpha();
	float x_offset = (.5*window_width + item_index*72.)*offset_scale*offset_scale;

	float x = base_x + x_offset - 32.*sinf(item->get_active_t()*M_PI);

	return g2d::vec2(x, y);
}

static const char *
level_sprite_name(int id)
{
	static char buf[80];
	sprintf(buf, "level-%d.png", id);
	return buf;
}

template <typename F, typename G>
struct level_menu_item : action_menu_item<F, G>
{
	level_menu_item(int level_id, F on_activated, G on_selected)
	: action_menu_item<F, G>(
		SOUND_MENU_VALIDATE,
		g2d::sprite_manager::get_instance().get_sprite(level_sprite_name(level_id*2)),
		g2d::sprite_manager::get_instance().get_sprite(level_sprite_name(level_id*2 + 1)),
		on_activated,
		on_selected,
		false)
	, level_id(level_id)
	{ }

	bool is_enabled() const
	{ return practice_mode || level_id <= cur_options->max_unlocked_level; }

	int level_id;
};

level_selection_menu::level_selection_menu(main_menu_impl *parent)
: parent(parent)
{
	g2d::sprite_manager& sm = g2d::sprite_manager::get_instance();

	for (int i = 0; i < NUM_LEVELS; i++) {
		auto on_activated = [this] { this->parent->set_state_outro(); };
		auto on_selected = [=] { start_in_game(i); };
		append_item(
			new level_menu_item<
				decltype(on_activated), decltype(on_selected)>
				(i, on_activated, on_selected));
	}

	append_action_item(
		SOUND_MENU_BACK,
		sm.get_sprite("back-sm-0.png"), sm.get_sprite("back-sm-1.png"),
		[this] { this->parent->show_background(); },
		[this] { this->parent->show_game_mode_menu(); },
		true);
}

g2d::vec2
level_selection_menu::get_item_position(int item_index) const
{
	const float y_origin = .5*(window_height + NUM_BUTTON_ROWS*ITEM_HEIGHT);

	const float offset_scale = 1. - get_cur_alpha();
	const float x_offset = (.5*window_width + item_index*96.)*offset_scale*offset_scale;

	if (item_index < NUM_LEVELS) {
		const float x_origin = .5*(window_width - NUM_BUTTON_COLS*ITEM_WIDTH);

		const int r = item_index/NUM_BUTTON_COLS;
		const int c = item_index%NUM_BUTTON_COLS;

		return g2d::vec2(x_origin + c*ITEM_WIDTH + x_offset,
			y_origin - (r + 1)*ITEM_HEIGHT);
	} else {
		const float x_origin = window_width - BACK_BUTTON_WIDTH;

		return g2d::vec2(x_origin + x_offset,
			y_origin - NUM_BUTTON_ROWS*ITEM_HEIGHT - 2*BACK_BUTTON_HEIGHT);
	}
}

namespace {

template <typename F, typename G>
class action_item_no_fade : public action_menu_item<F, G>
{
public:
	action_item_no_fade(
		int sound,
		const g2d::sprite *active_sprite,
		const g2d::sprite *inactive_sprite,
		F on_activation_fn,
		G on_action_fn,
		bool is_back)
	: action_menu_item<F, G>(sound, active_sprite, inactive_sprite, on_activation_fn, on_action_fn, is_back)
	{ }

	bool fade_menu_when_selected() const
	{ return false; }
};

} // anonymous namespace

void
main_menu_impl::create_main_menu()
{
	g2d::sprite_manager& sm = g2d::sprite_manager::get_instance();

	main_menu.append_action_item(
		SOUND_MENU_VALIDATE,
		sm.get_sprite("start-0.png"),
		sm.get_sprite("start-1.png"),
		[] { },
		[this] { set_cur_menu(&game_mode_menu); });

	main_menu.append_action_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("options-0.png"),
		sm.get_sprite("options-1.png"),
		[] { },
		[this] { set_cur_menu(&options_menu); });

	main_menu.append_action_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("more-0.png"),
		sm.get_sprite("more-1.png"),
		[] { },
		[this] { set_cur_menu(&more_menu); });

	main_menu.append_action_item(
		SOUND_MENU_BACK,
		sm.get_sprite("quit-0.png"),
		sm.get_sprite("quit-1.png"),
		[this] { cur_state = STATE_OUTRO; },
		[] { quit(); },
		true);
}

void
main_menu_impl::create_more_menu()
{
	g2d::sprite_manager& sm = g2d::sprite_manager::get_instance();

	more_menu.append_action_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("stats-0.png"),
		sm.get_sprite("stats-1.png"),
		[this] { hide_background(); },
		[] { start_stats_page(); });

	more_menu.append_action_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("credits-0.png"),
		sm.get_sprite("credits-1.png"),
		[this] { background.fg_billboard->hide(); start_sound(SOUND_LEVEL_INTRO, false); },
		[this] { start_credits(); });

	{
	auto dummy = [] { };

	more_menu.append_item(
		new action_item_no_fade<decltype(dummy), void(*)()>(
			SOUND_MENU_SELECT,
			sm.get_sprite("rate-me-0.png"),
			sm.get_sprite("rate-me-1.png"),
			dummy,
			on_rate_me_clicked,
			false));
	}

	more_menu.append_action_item(
		SOUND_MENU_BACK,
		sm.get_sprite("back-0.png"),
		sm.get_sprite("back-1.png"),
		[] { },
		[this] { show_root_menu(); },
		true);
}

void
main_menu_impl::create_options_menu()
{
	g2d::sprite_manager& sm = g2d::sprite_manager::get_instance();

	options_menu.append_toggle_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("hints-on-0.png"), sm.get_sprite("hints-on-1.png"),
		sm.get_sprite("hints-off-0.png"), sm.get_sprite("hints-off-1.png"),
		&cur_options->enable_hints,
		[] (int) { });

	options_menu.append_toggle_item(
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

	options_menu.append_action_item(
		SOUND_MENU_BACK,
		sm.get_sprite("back-0.png"), sm.get_sprite("back-1.png"),
		[] { },
		[this] { show_root_menu(); },
		true);
}

void
main_menu_impl::create_game_mode_menu()
{
	g2d::sprite_manager& sm = g2d::sprite_manager::get_instance();

	game_mode_menu.append_action_item(
		SOUND_MENU_VALIDATE,
		sm.get_sprite("challenge-0.png"), sm.get_sprite("challenge-1.png"),
		[this] { hide_background(); },
		[this] { practice_mode = false; set_cur_menu(&level_menu); });

	game_mode_menu.append_action_item(
		SOUND_MENU_VALIDATE,
		sm.get_sprite("practice-0.png"), sm.get_sprite("practice-1.png"),
		[this] { hide_background(); },
		[this] { practice_mode = true; set_cur_menu(&level_menu); });

	game_mode_menu.append_action_item(
		SOUND_MENU_VALIDATE,
		sm.get_sprite("tutorial-0.png"), sm.get_sprite("tutorial-1.png"),
		[this] { set_state_outro(); },
		[] { start_tutorial(); });

	game_mode_menu.append_action_item(
		SOUND_MENU_SELECT,
		sm.get_sprite("top-scores-0.png"),
		sm.get_sprite("top-scores-1.png"),
		[this] { hide_background(); },
		[this] { start_hiscore_list(); });

	game_mode_menu.append_action_item(
		SOUND_MENU_BACK,
		sm.get_sprite("back-0.png"), sm.get_sprite("back-1.png"),
		[] { },
		[this] { show_root_menu(); },
		true);
}

void
draw_fade_overlay(float alpha)
{
	render::set_blend_mode(blend_mode::ALPHA_BLEND);

	render::set_color({ 1.f, 1.f, 1.f, alpha*alpha });
	render::draw_quad({ { 0, 0 }, { 0, window_height }, { window_width, window_height }, { window_width, 0 } }, 10);
}

void
main_menu_impl::reset()
{
	background.reset();

#if 0
	if (cur_leaderboard->has_new_entry()) {
		cur_menu = &hiscore_menu;
		background.fg_billboard->set_state(title_widget::OUTSIDE);
		background.logo->set_state(title_widget::OUTSIDE);
	} else
#endif
	cur_menu = &main_menu;

	cur_menu->reset();

	fade_in();

	start_sound(SOUND_OPENING, false);
}

void
main_menu_impl::redraw() const
{
	background.draw();

	if (cur_state == STATE_FADE_IN) {
		draw_fade_overlay(1. - static_cast<float>(state_t)/FADE_IN_T);
	} else if (cur_state == STATE_OUTRO) {
		draw_fade_overlay(1. - cur_menu->get_cur_alpha());
	}

	cur_menu->draw();
}

void
main_menu_impl::update(uint32_t dt)
{
	state_t += dt;

	background.update(dt);

	if (cur_state == STATE_FADE_IN) {
		if (state_t >= FADE_IN_T)
			cur_state = STATE_ACTIVE;
	} else {
		cur_menu->update(dt);
	}
}

void
main_menu_impl::on_touch_down(float x, float y)
{
	touch_is_down = true;
	cur_menu->on_touch_down(x, y);
}

void
main_menu_impl::on_touch_up()
{
	touch_is_down = false;
	cur_menu->on_touch_up();
}

void
main_menu_impl::on_touch_move(float x, float y)
{
	if (touch_is_down)
		cur_menu->on_touch_down(x, y);
}

void
main_menu_impl::on_back_key()
{
	cur_menu->on_back_key();
}

void
main_menu_impl::on_menu_key()
{
}

main_menu_state::main_menu_state()
: impl_(new main_menu_impl)
{ }

main_menu_state::~main_menu_state()
{
	delete impl_;
}

void
main_menu_state::reset()
{
	impl_->reset();
}

void
main_menu_state::redraw() const
{
	impl_->redraw();
}

void
main_menu_state::update(uint32_t dt)
{
	impl_->update(dt);
}

void
main_menu_state::on_touch_down(float x, float y)
{
	impl_->on_touch_down(x, y);
}

void
main_menu_state::on_touch_up()
{
	impl_->on_touch_up();
}

void
main_menu_state::on_touch_move(float x, float y)
{
	impl_->on_touch_move(x, y);
}

void
main_menu_state::on_back_key()
{
	impl_->on_back_key();
}

void
main_menu_state::on_menu_key()
{
	impl_->on_menu_key();
}

void
main_menu_state::show_background()
{
	impl_->show_background();
}

void
main_menu_state::hide_background()
{
	impl_->hide_background();
}

void
main_menu_state::show_root_menu()
{
	impl_->show_root_menu();
}

void
main_menu_state::show_more_menu()
{
	impl_->show_more_menu();
}

void
main_menu_state::show_game_mode_menu()
{
	impl_->show_game_mode_menu();
}

void
main_menu_state::fade_in()
{
	impl_->fade_in();
}

void
main_menu_state::unfade()
{
	impl_->set_state_active();
}
