#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#include <SDL.h>
#else
#include <time.h>
#endif

#include "guava2d/font_manager.h"
#include "guava2d/g2dgl.h"
#include "guava2d/texture_manager.h"

#include "render.h"

#include "background.h"
#include "common.h"
#include "credits.h"
#include "hiscore_input.h"
#include "hiscore_list.h"
#include "http_request.h"
#include "in_game.h"
#include "in_game_menu.h"
#include "jukugo.h"
#include "kanji_info.h"
#include "kasui.h"
#include "main_menu.h"
#include "menu.h"
#include "options.h"
#include "panic.h"
#include "program_registry.h"
#include "settings.h"
#include "sprite_manager.h"
#include "stats_page.h"
#include "theme.h"
#include "tutorial.h"
#include "world.h"

float window_width, window_height;
int viewport_width, viewport_height;

options *cur_options;

#if 0
leaderboard *cur_leaderboard;
#endif

settings cur_settings;

int cur_level = 0;
bool practice_mode = false;
unsigned dpad_state;

static std::vector<state *> state_stack;

class kasui_impl
{
public:
    kasui_impl();
    ~kasui_impl();

    void resize(int width, int height);

    void redraw();

    void on_pause();
    void on_resume();

    void on_touch_down(int x, int y);
    void on_touch_up();
    void on_touch_move(int x, int y);

    void on_back_key_pressed();
    void on_menu_key_pressed();

    void add_http_request(http_request *req);

private:
    void initialize(int width, int height);
    void poll_http_requests();

    uint32_t prev_update_;
    bool initialized_;
    std::list<http_request *> http_requests_;
};

float frand()
{
    return (float)rand() / RAND_MAX;
}

int irand(int from, int to)
{
    return from + rand() % (to - from);
}

g2d::mat4 get_ortho_projection()
{
    return g2d::mat4::ortho(0, window_width, 0, window_height, -1, 1);
}

state *get_cur_state()
{
    assert(!state_stack.empty());
    return state_stack.back();
}

state *get_prev_state()
{
    return state_stack.size() > 1 ? state_stack[state_stack.size() - 2] : nullptr;
}

void push_state(state *next_state)
{
    next_state->reset();
    state_stack.push_back(next_state);
}

void pop_state()
{
    assert(!state_stack.empty());
    state_stack.pop_back();
}

// state constructors

state *get_main_menu_state()
{
    static main_menu_state instance;
    return &instance;
}

state *get_in_game_state()
{
    static in_game_state instance;
    return &instance;
}

static state *get_in_game_menu_state()
{
    static in_game_menu_state instance;
    return &instance;
}

static state *get_stats_page_state()
{
    static stats_page_state instance;
    return &instance;
}

static state *get_hiscore_list_state()
{
    static hiscore_list_state instance;
    return &instance;
}

static state *get_hiscore_input_state()
{
    static hiscore_input_state instance;
    return &instance;
}

static state *get_credits_state()
{
    static credits_state instance;
    return &instance;
}

static state *get_tutorial_state()
{
    static tutorial_state instance;
    return &instance;
}

void start_main_menu()
{
    push_state(get_main_menu_state());
}

void start_tutorial()
{
    push_state(get_tutorial_state());
}

void start_in_game(int level)
{
    cur_level = level;
    push_state(get_in_game_state());
}

void start_in_game_menu()
{
    push_state(get_in_game_menu_state());
}

void start_stats_page()
{
    push_state(get_stats_page_state());
}

void start_hiscore_list()
{
    push_state(get_hiscore_list_state());
}

void start_hiscore_input()
{
    push_state(get_hiscore_input_state());
}

void start_credits()
{
    push_state(get_credits_state());
}

void quit()
{
    cur_options->save(options_file_path);
#if 0
	cur_leaderboard->save(leaderboard_file_path);
#endif
    jukugo_save_hits(jukugo_hits_file_path);
    exit(0);
}

static void preload_resources()
{
    // textures

    static const char *textures[] = {
        "images/blocks.png", "images/clouds.png", "images/flare.png", "images/glow.png", "images/haru-bg.png",
        "images/petal.png",  "images/star.png",   "images/arrow.png", nullptr,
    };

    // fonts

    for (const char **p = textures; *p; p++)
        g2d::texture_manager::get_instance().load(*p);

    static const char *fonts[] = {
        "fonts/gameover", "fonts/large", "fonts/medium", "fonts/small",
        "fonts/tiny",     "fonts/title", "fonts/micro",  nullptr,
    };

    for (const char **p = fonts; *p; p++)
        g2d::font_manager::get_instance().load(*p);

    // programs

    get_program_instance<program_flat>();
    get_program_instance<program_color>();
    get_program_instance<program_texture_decal>();
    get_program_instance<program_texture_color>();
    get_program_instance<program_texture_alpha>();
    get_program_instance<program_texture_uniform_alpha>();
    get_program_instance<program_texture_uniform_color>();
    get_program_instance<program_text>();
    get_program_instance<program_text_outline>();
    get_program_instance<program_text_alpha>();
    get_program_instance<program_text_outline_alpha>();
    get_program_instance<program_text_gradient>();
    get_program_instance<program_text_outline_gradient>();
    get_program_instance<program_intro_text>();
    get_program_instance<program_timer_text>();
    get_program_instance<program_3d_texture_color>();
    get_program_instance<program_grid_background>();

    // sprites

    g2d::load_sprite_sheet("sprites/sprites");
}

static uint32_t get_cur_tics()
{
#ifdef _WIN32
    return SDL_GetTicks();
#else
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
#endif
}

static void initialize_options()
{
    if (!(cur_options = options::load(options_file_path)))
        cur_options = new options;
}

#if 0
static void
initialize_leaderboard()
{
	if (!(cur_leaderboard = leaderboard::load(leaderboard_file_path))) {
		cur_leaderboard = new leaderboard;

		for (int i = leaderboard::MAX_ENTRIES; i > 0; --i)
			cur_leaderboard->add_entry(i, false);
	}
}
#endif

kasui_impl::kasui_impl()
    : initialized_(false)
{
}

kasui_impl::~kasui_impl()
{
}

void kasui_impl::on_pause()
{
    cur_options->save(options_file_path);
#if 0
	cur_leaderboard->save(leaderboard_file_path);
#endif
    jukugo_save_hits(jukugo_hits_file_path);
}

void kasui_impl::on_resume()
{
}

void kasui_impl::resize(int width, int height)
{
    if (!initialized_) {
        initialize(width, height);
    } else {
        // resuming after pause

        g2d::texture_manager::get_instance().load_all();
        reload_all_programs();
    }

    glViewport(0, 0, viewport_width, viewport_height);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    prev_update_ = 0;
}

void kasui_impl::initialize(int width, int height)
{
    // resize

    viewport_width = width;
    viewport_height = height;

    window_width = 640.;
    window_height = static_cast<float>(height) * window_width / width;

    int downsample_scale = 1;

    g2d::texture_manager::get_instance().set_downsample_scale(downsample_scale);
    preload_resources();

    render::init();

    // initialize game

    load_settings();
    initialize_options();
#if 0
	initialize_leaderboard();
#endif

    jukugo_initialize();
    jukugo_load_hits(jukugo_hits_file_path);
    kanji_info_initialize();

    world_init();

    background_initialize();

    // states

    get_main_menu_state();
    get_in_game_state();
    get_in_game_menu_state();
    get_stats_page_state();
    get_hiscore_list_state();
    get_credits_state();
    get_tutorial_state();

    start_main_menu();
// start_hiscore_list();

#if 0
	static_cast<main_menu_state *>(get_cur_state())->hide_background();
	start_hiscore_input();
	static_cast<hiscore_input_state *>(get_cur_state())->set_score(41414);
#endif
    // start_in_game(0);
    // start_stats_page();
    // start_tutorial();

    // start_in_game(0);
    // get_cur_state()->on_back_key();

    initialized_ = true;
}

void kasui_impl::redraw()
{
#ifndef DUMP_FRAMES
    const uint32_t now = get_cur_tics();

    uint32_t dt;

    if (prev_update_ == 0 || now < prev_update_)
        dt = 0;
    else
        dt = now - prev_update_;

    static const uint32_t MAX_DT = 1000 / 20;

    while (dt > MAX_DT) {
        get_cur_state()->update(MAX_DT);
        dt -= MAX_DT;
    }

    get_cur_state()->update(dt);

    poll_http_requests();

    render::begin_batch();
    render::set_viewport(0, window_width, 0, window_height);

    get_cur_state()->redraw();

    render::end_batch();

    prev_update_ = now;
#else
    update();
    redraw(0);
#endif
}

void kasui_impl::add_http_request(http_request *req)
{
    http_requests_.push_back(req);
}

void kasui_impl::poll_http_requests()
{
    auto it = http_requests_.begin();

    while (it != http_requests_.end()) {
        auto *p = *it;

        if (!p->poll()) {
            delete p;
            it = http_requests_.erase(it);
        } else {
            ++it;
        }
    }
}

void kasui_impl::on_touch_down(int x, int y)
{
    float sx = x * window_width / viewport_width;
    float sy = (viewport_height - 1 - y) * window_height / viewport_height;

    get_cur_state()->on_touch_down(sx, sy);
}

void kasui_impl::on_touch_up()
{
    get_cur_state()->on_touch_up();
}

void kasui_impl::on_touch_move(int x, int y)
{
    float sx = x * window_width / viewport_width;
    float sy = (viewport_height - 1 - y) * window_height / viewport_height;

    get_cur_state()->on_touch_move(sx, sy);
}

void kasui_impl::on_back_key_pressed()
{
    get_cur_state()->on_back_key();
}

void kasui_impl::on_menu_key_pressed()
{
    get_cur_state()->on_menu_key();
}

kasui::kasui()
    : impl_(new kasui_impl)
{
}

kasui::~kasui()
{
    delete impl_;
}

void kasui::resize(int width, int height)
{
    impl_->resize(width, height);
}

void kasui::redraw()
{
    impl_->redraw();
}

void kasui::on_pause()
{
    impl_->on_pause();
}

void kasui::on_resume()
{
    impl_->on_resume();
}

void kasui::on_touch_down(int x, int y)
{
    impl_->on_touch_down(x, y);
}

void kasui::on_touch_up()
{
    impl_->on_touch_up();
}

void kasui::on_touch_move(int x, int y)
{
    impl_->on_touch_move(x, y);
}

void kasui::on_back_key_pressed()
{
    impl_->on_back_key_pressed();
}

void kasui::on_menu_key_pressed()
{
    impl_->on_menu_key_pressed();
}

void kasui::add_http_request(http_request *req)
{
    impl_->add_http_request(req);
}

kasui &kasui::get_instance()
{
    static kasui the_instance;
    return the_instance;
}
