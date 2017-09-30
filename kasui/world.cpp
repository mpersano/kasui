#include <cassert>

#include <algorithm>

#include <wchar.h>
#include <stdlib.h>
#include <string.h>

#include <guava2d/rgb.h>
#include <guava2d/texture_manager.h>
#include <guava2d/font_manager.h>
#include <guava2d/vertex_array.h>
#include <guava2d/program.h>
#include <guava2d/xwchar.h>

#include "render.h"
#include "common.h"
#include "panic.h"
#include "tween.h"
#include "jukugo.h"
#include "settings.h"
#include "program_registry.h"
#include "jukugo_info_sprite.h"
#include "bakudan_sprite.h"
#include "combo_sprite.h"
#include "hint_animation.h"
#include "sounds.h"
#include "world.h"

#include "block_info.cpp"

namespace {

enum {
	DEAD_BLOCK_TTL = 30*MS_PER_TIC,

	FLARE_TEXTURE_ROWS = 4,
	FLARE_NUM_FRAMES = FLARE_TEXTURE_ROWS*FLARE_TEXTURE_ROWS,
	FLARE_FRAME_SIZE = 256,
	FLARE_TICS = FLARE_NUM_FRAMES*MS_PER_TIC,
};

#define NUM_BLOCK_TYPES (sizeof block_infos/sizeof *block_infos - 1)
static jukugo *match_map[NUM_BLOCK_TYPES][NUM_BLOCK_TYPES];

int
get_block_index_by_kanji(wchar_t ch)
{
	for (unsigned i = 0; i < NUM_BLOCK_TYPES; i++) {
		const block_info *p = &block_infos[i];

		if (p->kanji == ch)
			return i;
	}

	return -1;
}

void
initialize_match_map()
{
	for (unsigned i = 0; i < jukugo_list_size; i++) {
		jukugo *p = jukugo_list[i];

		const int i0 = get_block_index_by_kanji(p->kanji[0]);
		const int i1 = get_block_index_by_kanji(p->kanji[1]);
		assert(i0 != -1 && i1 != -1);

		match_map[i0][i1] = p;
	}
}

bool
rand_bakudan()
{
	return (rand() % cur_settings.game.bakudan_period) == 0;
}

class dead_block_sprite : public sprite
{
public:
	dead_block_sprite(
		const g2d::vec2& pos,
		float cell_size,
		const g2d::texture *texture,
		const g2d::vec2& uv0,
		const g2d::vec2& uv1,
		const g2d::rgb& color)
	: pos_(pos)
	, speed_(g2d::vec2(-.5 + static_cast<float>(rand())/RAND_MAX, .5 + static_cast<float>(rand())/RAND_MAX))
	, cell_size_(cell_size)
	, texture_(texture)
	, uv0_(uv0)
	, uv1_(uv1)
	, color_(color)
	, ttl_(DEAD_BLOCK_TTL)
	{
		speed_.set_length(10.f/MS_PER_TIC);
	}

	bool update(uint32_t dt);
	void draw() const;

private:
	g2d::vec2 pos_;
	g2d::vec2 speed_;
	float cell_size_;
	const g2d::texture *texture_;
	g2d::vec2 uv0_, uv1_;
	g2d::rgb color_;
	int ttl_;
};

bool
dead_block_sprite::update(uint32_t dt)
{
	if ((ttl_ -= dt) <= 0)
		return false;

	pos_ += dt*speed_;
	speed_.y -= dt*.8f/(MS_PER_TIC*MS_PER_TIC);

	return true;
}

void
dead_block_sprite::draw() const
{
	const g2d::vec2 p = pos_;

	const float x0 = p.x;
	const float x1 = x0 + cell_size_;

	const float y0 = p.y;
	const float y1 = y0 + cell_size_;

	const float u0 = uv0_.y;
	const float u1 = uv1_.y;

	const float v0 = uv0_.x;
	const float v1 = uv1_.x;

	const float alpha = static_cast<float>(ttl_)/DEAD_BLOCK_TTL;

	render::set_blend_mode(blend_mode::ALPHA_BLEND);
	render::set_color({ color_.r/255.f, color_.g/255.f, color_.b/255.f, alpha });
	render::draw_quad(
		texture_,
		{ { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } },
		{ { u0, v1 }, { u1, v1 }, { u1, v0 }, { u0, v0 } },
		10);
}

class drop_trail_sprite : public sprite
{
public:
	drop_trail_sprite(
		const g2d::vec2& pos,
		float cell_size,
		const g2d::texture *texture,
		const g2d::vec2& uv0,
		const g2d::vec2& uv1,
		const g2d::rgb& color)
	: pos_(pos)
	, cell_size_(cell_size)
	, texture_(texture)
	, uv0_(uv0)
	, uv1_(uv1)
	, color_(color)
	, tics_(0)
	{ }

	bool update(uint32_t dt);
	void draw() const;

private:
	g2d::vec2 pos_;
	float cell_size_;
	const g2d::texture *texture_;
	g2d::vec2 uv0_, uv1_;
	g2d::rgb color_;
	int tics_;

	enum { TTL = 40*MS_PER_TIC };
};

bool
drop_trail_sprite::update(uint32_t dt)
{
	return (tics_ += dt) < TTL;
}

void
drop_trail_sprite::draw() const
{
	enum {
		NUM_COMPONENTS = 30
	};

	float lerp_factor = 1. - static_cast<float>(tics_)/TTL;
	if (lerp_factor < 0)
		lerp_factor = 0;

	static g2d::vertex_array_texuv_color gv(NUM_COMPONENTS*6);
	gv.reset();

	float alpha = .5*lerp_factor;

	const float r = color_.r/255.f;
	const float g = color_.g/255.f;
	const float b = color_.b/255.f;

	const float u0 = uv0_.y;
	const float u1 = uv1_.y;

	const float v0 = uv0_.x;
	const float v1 = uv1_.x;

	render::set_blend_mode(blend_mode::ALPHA_BLEND);

	for (int i = 0; i < NUM_COMPONENTS; i++) {
		const float x0 = pos_.x;
		const float x1 = x0 + cell_size_;

		const float y0 = pos_.y + .5*i*lerp_factor*cell_size_;
		const float y1 = y0 + cell_size_;

		render::set_color({ r, g, b, alpha });
		render::draw_quad(
			texture_,
			{ { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } },
			{ { u0, v1 }, { u1, v1 }, { u1, v0 }, { u0, v0 } },
			10);

		alpha *= .8;
	}
}

class explosion_particles : public sprite
{
public:
	explosion_particles(const g2d::vec2& pos, const gradient *g);

	bool update(uint32_t dt);
	void draw() const;

private:
	struct particle {
		bool is_active() const
		{ return tics < ttl; }

		int ttl, tics;
		float radius;
		g2d::vec2 pos, speed;
		float angle, delta_angle;
		g2d::rgb color;
	};

	const g2d::texture *texture_;
	static const int NUM_PARTICLES = 32;
	particle particles_[NUM_PARTICLES];
};

explosion_particles::explosion_particles(const g2d::vec2& pos, const gradient *g)
: texture_(g2d::texture_manager::get_instance().load("images/star.png"))
{
	const float f = 1./MS_PER_TIC;

	for (auto& p : particles_) {
		p.tics = 0;
		p.ttl = irand(20, 50)*MS_PER_TIC;
		p.radius = frand(10., 20.);
		p.pos = pos;

		float ang = frand(.15, M_PI - .15);
		p.speed = f*frand(3., 5.)*g2d::vec2(cosf(ang), sinf(ang));

		p.angle = frand(0., 2.*M_PI);
		p.delta_angle = f*frand(-.15, .15);

		g2d::rgb color = frand(*g->from, *g->to) + g2d::rgb(60, 60, 60);

		p.color.r = std::min(static_cast<int>(color.r), 255);
		p.color.g = std::min(static_cast<int>(color.g), 255);
		p.color.b = std::min(static_cast<int>(color.b), 255);
	}
}

bool
explosion_particles::update(uint32_t dt)
{
	bool is_active = false;

	for (auto& p : particles_) {
		if (!p.is_active() || (p.tics += dt) >= p.ttl)
			continue;

		p.pos += dt*p.speed;
		p.speed += dt*g2d::vec2(0, -.15)/(MS_PER_TIC*MS_PER_TIC);
		p.angle += dt*p.delta_angle;

		is_active = true;
	}

	return is_active;
}

void
explosion_particles::draw() const
{
	for (const auto& p : particles_) {
		if (!p.is_active())
			continue;

		int fade_tic = .8*p.ttl;

		float a;

		if (p.tics < FLARE_TICS)
			a = static_cast<float>(p.tics)/FLARE_TICS;
		else if (p.tics < fade_tic)
			a = 1.f;
		else
			a = 1. - static_cast<float>(p.tics - fade_tic)/(p.ttl - fade_tic);

		a *= .6;

		const g2d::vec2& pos = p.pos;

		const float c = cosf(p.angle);
		const float s = sinf(p.angle);

		const g2d::vec2 right = p.radius*g2d::vec2(c, s);
		const g2d::vec2 up = p.radius*g2d::vec2(-s, c);

		const g2d::vec2 p0 = pos + up - right;
		const g2d::vec2 p1 = pos + up + right;
		const g2d::vec2 p2 = pos - up + right;
		const g2d::vec2 p3 = pos - up - right;

		const float r = p.color.r/255.f;
		const float g = p.color.g/255.f;
		const float b = p.color.b/255.f;

		render::set_color({ r, g, b, a });
		render::draw_quad(
			texture_,
			{ { p0.x, p0.y }, { p1.x, p1.y }, { p2.x, p2.y }, { p3.x, p3.y } },
			{ { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } },
			5);
	}
}

} // anonymous namespace

void
world_init()
{
	initialize_match_map();
}

falling_block::falling_block(world& w)
: world_(w)
{ }

void
falling_block::initialize()
{
	const int num_level_block_types = world_.get_num_level_block_types();

	row = world_.get_num_rows();
	col = (world_.get_num_cols() - 1)/2;

	block_types[0] = rand() % num_level_block_types;

	block_types[1] = -1;
	int index = 1;

	for (int i = 0; i < num_level_block_types; i++) {
		if (!match_map[block_types[0]][i] && !match_map[i][block_types[0]]) {
			if ((rand() % index) == 0)
				block_types[1] = i;
			++index;
		}
	}

	assert(block_types[1] != -1);

	if (rand_bakudan())
		block_types[rand() % 2] |= BAKUDAN_FLAG;

	tics_to_drop = cur_settings.game.tics_to_drop;

	state_flags = DROPPING|FADING_IN;
	drop_tics = 0;

	is_active = true;
}

void
falling_block::get_block_positions(g2d::vec2& p0, g2d::vec2& p1) const
{
	const float cell_size = world_.get_cell_size();

	float y = row*cell_size;
	float x = col*cell_size;

	if (is_dropping()) {
		const float s = static_cast<float>(drop_tics)/(cur_settings.animation.drop_tics*MS_PER_TIC);
		y -= s*cell_size;
	}

	if (is_moving()) {
		const float s = static_cast<float>(move_tics)/(cur_settings.animation.move_tics*MS_PER_TIC);
		x += s*cell_size*move_dir;
	}

	if (is_swapping()) {
		float a = (M_PI*swap_tics)/(cur_settings.animation.swap_tics*MS_PER_TIC);

		float c = .5*cell_size*cosf(a);
		float s = .5*cell_size*sinf(a);

		float x0 = x + cell_size;
		float y0 = y + .5*cell_size;

		p0 = g2d::vec2(x0 - c - .5*cell_size, y0 - s - .5*cell_size);
		p1 = g2d::vec2(x0 + c - .5*cell_size, y0 + s - .5*cell_size);
	} else {
		p0 = g2d::vec2(x, y);
		p1 = g2d::vec2(x + cell_size, y);
	}
}

void
falling_block::draw() const
{
	const float cell_size = world_.get_cell_size();

	g2d::vec2 p0, p1;
	get_block_positions(p0, p1);

	float alpha;

	if (is_fading_in())
		alpha = static_cast<float>(drop_tics)/(cur_settings.animation.drop_tics*MS_PER_TIC);
	else
		alpha = 1;

	// draw blocks

	world_.draw_block(block_types[0], p0.x, p0.y, alpha);
	world_.draw_block(block_types[1], p1.x, p1.y, alpha);

	// draw block shadows

	for (int i = 0; i < 2; i++) {
		int r = row;
		while (r > 0 && world_.get_block_at(r - 1, col + i) == 0)
			--r;
		world_.draw_block(block_types[i], (col + i)*cell_size, r*cell_size, .25*alpha, g2d::rgb(.5, .5, .5));
	}
}

bool
falling_block::can_drop() const
{
	if (row == 0)
		return false;

	return !(world_.get_block_at(row - 1, col) || world_.get_block_at(row - 1, col + 1));
}

void
falling_block::drop()
{
	if (!can_drop()) {
		assert(col >= 0 && col <= world_.get_num_cols() - 2);
		assert(world_.get_block_at(row, col) == 0 && world_.get_block_at(row, col + 1) == 0);

		world_.set_block_at(row, col, block_types[0] + 1);
		world_.set_block_at(row, col + 1, block_types[1] + 1);

		is_active = false;
	} else {
		set_is_dropping();
	}
}

void
falling_block::drop_fast()
{
	for (int i = 0; i < 2; i++) {
		int r = row;
		while (r > 0 && world_.get_block_at(r - 1, col + i) == 0)
			--r;
		world_.set_block_at(r, col + i, block_types[i] + 1);
		world_.spawn_drop_trail(r, col + i, block_types[i]);
	}

	is_active = false;
}

bool
falling_block::on_left_pressed()
{
	if (is_active && !is_moving()) {
		move_left();
		return true;
	} else {
		return false;
	}
}

bool
falling_block::on_right_pressed()
{
	if (is_active && !is_moving()) {
		move_right();
		return true;
	} else {
		return false;
	}
}

bool
falling_block::on_up_pressed()
{
	if (is_active && !is_swapping()) {
		set_is_swapping();
		return true;
	} else {
		return false;
	}
}

bool
falling_block::on_down_pressed()
{
	if (is_active && !is_fading_in()) {
		drop_fast();
		return true;
	} else {
		return false;
	}
}

void
falling_block::update(uint32_t dt)
{
	if (!is_active)
		return;

	tics_to_drop -= dt;

	if (is_swapping()) {
		if ((swap_tics += dt) >= cur_settings.animation.swap_tics*MS_PER_TIC) {
			int t = block_types[0];
			block_types[0] = block_types[1];
			block_types[1] = t;
			unset_is_swapping();
		}
	}

	if (is_moving()) {
		if ((move_tics += dt) >= cur_settings.animation.move_tics*MS_PER_TIC) {
			col += move_dir;
			unset_is_moving();
		}
	}

	if (is_dropping()) {
		if ((drop_tics += dt) >= cur_settings.animation.drop_tics*MS_PER_TIC) {
			--row;
			tics_to_drop = cur_settings.game.tics_to_drop*MS_PER_TIC;
			unset_is_dropping();
			unset_is_fading_in();
		}
	}

	if (!is_dropping() && !is_moving() && tics_to_drop <= 0)
		drop();
}

void
falling_block::move_left()
{
	assert(!is_moving());

	int r = is_dropping() ? row - 1 : row;
	int c = col;

	while (c > 0 && world_.get_block_at(r, c - 1) == 0)
		--c;

	if (col > c) {
		move_dir = -1;
		set_is_moving();
	}
}

void
falling_block::move_right()
{
	assert(!is_moving());

	int r = is_dropping() ? row - 1 : row;
	int c = col;

	while (c < world_.get_num_cols() - 2 && world_.get_block_at(r, c + 2) == 0)
		++c;

	if (col < c) {
		move_dir = 1;
		set_is_moving();
	}
}

const wchar_t *
falling_block::get_kanji_text() const
{
	static wchar_t text[3] = {0};

	text[0] = block_infos[block_types[0] & ~BAKUDAN_FLAG].kanji;
	text[1] = block_infos[block_types[1] & ~BAKUDAN_FLAG].kanji;

	return text;
}

void
falling_block::inactivate()
{
	g2d::vec2 p0, p1;

	get_block_positions(p0, p1);

	world_.spawn_dead_block_sprite(p0, block_types[0]);
	world_.spawn_dead_block_sprite(p1, block_types[1]);

	is_active = false;
}

#define CUR_FALLING_BLOCK (&falling_block_queue_[falling_block_index_])

world::world(int rows, int cols, int wanted_height)
: practice_mode_(false)
, rows_(rows)
, cols_(cols)
, grid_(new int[rows_*cols_]) 
, matches_(new bool[rows_*cols_])
, blocks_texture_(g2d::texture_manager::get_instance().load("images/blocks.png"))
, flare_texture_(g2d::texture_manager::get_instance().load("images/flare.png"))
, falling_block_queue_ { *this, *this }
, event_listener_(nullptr)
{
	const float wanted_cell_size = wanted_height/rows_;
	const float max_cell_size = .9*window_width/cols_;
	cell_size_ = wanted_cell_size < max_cell_size ? wanted_cell_size : max_cell_size;

	memset(grid_, 0, rows_*cols_*sizeof *grid_);

	init_background_va();

	reset();
}

world::~world()
{
	delete[] grid_;
	delete[] matches_;
}

#include "theme.h"

void
world::reset()
{
	score_ = 0;

	std::for_each(
		sprites_.begin(),
		sprites_.end(),
		[] (sprite *p) { delete p; });

	sprites_.clear();
}

void
world::set_level(int level, bool practice_mode, bool enable_hints)
{
	practice_mode_ = practice_mode;
	enable_hints_ = enable_hints;

	level_score_delta_ = 13 + 31*level;

	level_jukugo_left_ = 5 + cur_level*2;
	if (level_jukugo_left_ > 12)
		level_jukugo_left_ = 12;
	// hud->set_jukugo_left(level_jukugo_left);

	num_level_block_types_ = NUM_NEW_KANJI_PER_LEVEL*(level + 1);
	if (num_level_block_types_ >= NUM_BLOCK_TYPES)
		num_level_block_types_ = NUM_BLOCK_TYPES;

	falling_block_index_ = 0;
	falling_block_queue_[0].initialize();
	falling_block_queue_[1].initialize();

	set_state_before_falling_block();
}

void
world::set_enable_hints(bool enable)
{
	enable_hints_ = enable;
}

void
world::set_falling_blocks(wchar_t left, wchar_t right)
{
	if ((CUR_FALLING_BLOCK->block_types[0] = get_block_index_by_kanji(left)) == -1)
		panic("%s: invalid kanji", __func__);

	if ((CUR_FALLING_BLOCK->block_types[1] = get_block_index_by_kanji(right)) == -1)
		panic("%s: invalid kanji", __func__);
}

void
world::set_row(int row_index, const wchar_t *kanji)
{
	if (row_index < 0 || row_index >= rows_)
		panic("%s: invalid row number %d", __func__, row_index);

	int *row = &grid_[row_index*cols_];
	const int *end = &row[cols_];

	for (const wchar_t *p = kanji; *p; p++) {
		int block = 0;

		if (*p == '*') {
			if (!*++p)
				break;
			block = BAKUDAN_FLAG;
		}

		block |= get_block_index_by_kanji(*p) + 1;

		*row++ = block;

		if (row == end)
			break;
	}
}

void
world::initialize_grid(int num_filled_rows)
{
	memset(grid_, 0, rows_*cols_*sizeof *grid_);

	for (int i = 0; i < num_filled_rows; i++) {
		for (int j = 0; j < cols_; j++) {
			int index = 1, block_index = -1;

			for (int k = 0; k < num_level_block_types_; k++) {
				if (j > 0 && match_map[get_block_type_at(i, j - 1) - 1][k])
					continue;

				if (i > 0 && match_map[k][get_block_type_at(i - 1, j) - 1])
					continue;

				if ((rand() % index) == 0)
					block_index = k;

				++index;
			}

			assert(block_index != -1);

			int v = block_index + 1;

			if (rand_bakudan())
				v |= BAKUDAN_FLAG;

			set_block_at(i, j, v);
		}
	}
}

void
world::set_theme_colors(const g2d::rgb& color, const g2d::rgb& opposite_color)
{
	theme_color_ = color;
	theme_opposite_color_ = opposite_color;
}

void
world::set_text_gradient(const gradient *g)
{
	text_gradient_ = g;
}

void
world::set_state(game_state next_state)
{
	cur_state_ = next_state;
	state_tics_ = 0;
}

void
world::initialize_dropping_blocks()
{
	num_dropping_blocks_ = 0;

	for (int c = 0; c < cols_; c++) {
		bool empty = false;
		int dest_row = 0;

		for (int r = 0; r < rows_; r++) {
			if (int t = get_block_at(r, c)) {
				if (empty) {
					auto& p = dropping_blocks_[num_dropping_blocks_++];

					p.col_ = c;
					p.type_ = t - 1;
					p.height_ = r*cell_size_;
					p.dest_height_ = dest_row*cell_size_;
					p.speed_ = frand(0., .5/MS_PER_TIC);
					p.active_ = true;
				}

				++dest_row;
			} else {
				empty = true;
			}
		}
	}
}

bool
world::find_matches()
{
	float y_offset = 0;

	memset(matches_, 0, rows_*cols_*sizeof *matches_);

	bool found = false;

#define MATCH(r, c) matches_[(r)*cols_ + (c)]

	for (int r = rows_ - 1; r >= 0; r--) {
		for (int c = 0; c < cols_; c++) {
			int j0;

			if ((j0 = get_block_type_at(r, c)) != 0) {
				int j1;
				jukugo *p;

				if (c < cols_ - 1 && (j1 = get_block_type_at(r, c + 1)) != 0) {
					if ((p = match_map[j0 - 1][j1 - 1])) {
						MATCH(r, c) = MATCH(r, c + 1) = true;

						const float x = (c + 1)*cell_size_;
						const float y = (r + .5)*cell_size_ + y_offset;
						sprites_.push_front(
							new jukugo_info_sprite(p, x, y, text_gradient_));

						if (!practice_mode_)
							p->hits++;

						found = true;

						y_offset += 2*cell_size_;
					}
				}

				if (r > 0 && (j1 = get_block_type_at(r - 1, c)) != 0) {
					if ((p = match_map[j0 - 1][j1 - 1])) {
						MATCH(r, c) = MATCH(r - 1, c) = true;

						const float x = (c + .5)*cell_size_;
						const float y = r*cell_size_ + y_offset;
						sprites_.push_front(
							new jukugo_info_sprite(p, x, y, text_gradient_));

						if (!practice_mode_)
							p->hits++;

						found = true;
						y_offset += 2*cell_size_;
					}
				}
			}
		}
	}

#undef MATCH

	if (found) {
		// start bakudan/particle animations

		for (int i = 0; i < rows_*cols_; i++) {
			if (matches_[i]) {
				const int r = i/cols_;
				const int c = i%cols_;

				const float x = (c + .5)*cell_size_;
				const float y = (r + .5)*cell_size_;

				if ((grid_[i] & BAKUDAN_FLAG))
					sprites_.push_front(new bakudan_sprite(x, y));

				sprites_.push_front(new explosion_particles(g2d::vec2(x, y), text_gradient_));
			}
		}

		start_sound(SOUND_BLOCK_MATCH, false);
	}

	return found;
}

void
world::solve_matches()
{
	// make sure find_matches was called before this!

	for (int r = rows_ - 1; r >= 0; r--) {
		for (int c = 0; c < cols_; c++) {
			if (int j0 = get_block_type_at(r, c)) {
				int j1;

				if (c < cols_ - 1 && (j1 = get_block_type_at(r, c + 1)) != 0) {
					const jukugo *p = match_map[j0 - 1][j1 - 1];
					if (p) {
						++combo_size_;
						score_ += score_delta_;

						if (event_listener_)
							event_listener_->set_score(score_);

						score_delta_ *= 4/3;

						if (!practice_mode_ && level_jukugo_left_ > 0) {
							--level_jukugo_left_;
							if (event_listener_)
								event_listener_->set_jukugo_left(level_jukugo_left_);
						}
					}
				}

				if (r > 0 && (j1 = get_block_type_at(r - 1, c)) != 0) {
					const jukugo *p = match_map[j0 - 1][j1 - 1];
					if (p) {
						++combo_size_;

						score_ += score_delta_;

						if (event_listener_)
							event_listener_->set_score(score_);

						score_delta_ *= 4/3;

						if (!practice_mode_ && level_jukugo_left_ > 0) {
							--level_jukugo_left_;
							if (event_listener_)
								event_listener_->set_jukugo_left(level_jukugo_left_);
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < rows_*cols_; i++) {
		if (grid_[i] && matches_[i]) {

#define KILL_BLOCK(i) \
	const int r = i/cols_; \
	const int c = i%cols_; \
	spawn_dead_block_sprite(cell_size_*g2d::vec2(c, r), get_block_at(r, c) - 1); \
	set_block_at(r, c, 0);

			if (!(grid_[i] & BAKUDAN_FLAG)) {
				KILL_BLOCK(i)
			} else {
				int block_type = grid_[i] & ~BAKUDAN_FLAG;

				for (int j = 0; j < rows_*cols_; j++) {
					if ((grid_[j] & ~BAKUDAN_FLAG) == block_type) {
						KILL_BLOCK(j)
					}
				}
			}
#undef KILL_BLOCK
		}
	}

	if (combo_size_ > 1)
		sprites_.push_back(new combo_sprite(combo_size_, 0, .6*rows_*cell_size_, text_gradient_));
}


bool
world::has_hanging_blocks() const
{
	for (int *p = grid_; p != &grid_[(rows_ - 1)*cols_]; p++) {
		if (p[0] == 0 && p[cols_])
			return true;
	}

	return false;
}

bool
world::is_game_over() const
{
	const int col = (cols_ - 1)/2;
	const int row = rows_ - 1;
	return get_block_at(row, col) || get_block_at(row, col + 1);
}

bool
world::is_level_completed() const
{
	return !practice_mode_ && level_jukugo_left_ == 0;
}

void
world::next_falling_block()
{
	CUR_FALLING_BLOCK->initialize();
	falling_block_index_ ^= 1;
}

void
world::set_falling_block_on_listener(const falling_block& p) const
{
	if (event_listener_) {
		wchar_t left = block_infos[p.block_types[0] & ~BAKUDAN_FLAG].kanji;
		wchar_t right = block_infos[p.block_types[1] & ~BAKUDAN_FLAG].kanji;

		event_listener_->set_next_falling_blocks(left, right);
	}
}

void
world::set_state_before_falling_block()
{
	set_state(STATE_BEFORE_FALLING_BLOCK);
	set_falling_block_on_listener(falling_block_queue_[falling_block_index_]);
}

void
world::set_state_falling_block()
{
	combo_size_ = 0;
	score_delta_ = level_score_delta_;

	set_state(STATE_FALLING_BLOCK);

	set_falling_block_on_listener(falling_block_queue_[falling_block_index_ ^ 1]);

	if (event_listener_)
		event_listener_->on_falling_blocks_started();
}

void
world::set_state_falling_block_or_hint()
{
	hint h;

	if (enable_hints_ && (rand() % cur_settings.game.hint_period) == 0 && get_hint(h)) {
		// hint

		hint_r_ = h.match_r;
		hint_c_ = h.match_c;

		hint_text_box *box = new hint_text_box(h, cell_size_, .85*get_width(), text_gradient_);

		const g2d::vec2 to_pos = cell_size_*g2d::vec2(h.block_c, h.block_r);

		float base_x = to_pos.x + .5*cell_size_;

		const float margin = .125*get_width();

		const float min_x = -margin;
		const float max_x = get_width() + margin;

		if (base_x - .5*box->get_width() < min_x)
			base_x = min_x + .5*box->get_width();
		else if (base_x + .5*box->get_width() > max_x)
			base_x = max_x - .5*box->get_width();

		if (to_pos.y < .5*get_height())
			box->set_pos(g2d::vec2(base_x, to_pos.y + cell_size_ + .5*box->get_height()));
		else
			box->set_pos(g2d::vec2(base_x, to_pos.y - cell_size_ - .5*box->get_height()));

		sprites_.push_front(box);

		set_state(STATE_HINT);
	} else {
		set_state_falling_block();
	}
}

void
world::set_state_dropping_hanging()
{
	initialize_dropping_blocks();
	set_state(STATE_DROPPING_HANGING);
}

bool
world::on_left_pressed()
{
	if (cur_state_ == STATE_FALLING_BLOCK) {
		return CUR_FALLING_BLOCK->on_left_pressed();
	} else {
		return false;
	}
}

bool
world::on_right_pressed()
{
	if (cur_state_ == STATE_FALLING_BLOCK) {
		return CUR_FALLING_BLOCK->on_right_pressed();
	} else {
		return false;
	}
}

bool
world::on_up_pressed()
{
	if (cur_state_ == STATE_FALLING_BLOCK) {
		return CUR_FALLING_BLOCK->on_up_pressed();
	} else if (cur_state_ == STATE_HINT) {
		return static_cast<hint_text_box *>(sprites_.front())->close();
	} else {
		return false;
	}
}

bool
world::on_down_pressed()
{
	if (cur_state_ == STATE_FALLING_BLOCK) {
		return CUR_FALLING_BLOCK->on_down_pressed();
	} else {
		return false;
	}
}

void
world::set_block_kanji_at(int row, int col, wchar_t kanji)
{
	set_block_at(row, col, get_block_index_by_kanji(kanji));
}

void
world::set_game_over()
{
	if (cur_state_ == STATE_FALLING_BLOCK)
		CUR_FALLING_BLOCK->inactivate();

	set_state(STATE_GAME_OVER);
}

void
world::update_animations(uint32_t dt)
{
        auto it = sprites_.begin();

        while (it != sprites_.end()) {
                sprite *p = *it;

                if (!p->update(dt)) {
                        delete p;
                        it = sprites_.erase(it);
                } else {
                        ++it;
                }
        }
}

void
world::update(uint32_t dt)
{
	update_animations(dt);

	state_tics_ += dt;

	switch (cur_state_) {
		case STATE_BEFORE_FALLING_BLOCK:
			set_state_falling_block_or_hint();
			break;

		case STATE_HINT:
			if (sprites_.empty())
				set_state_falling_block();
			break;

		case STATE_FALLING_BLOCK:
			CUR_FALLING_BLOCK->update(dt);

			if (!CUR_FALLING_BLOCK->get_is_active()) {
				next_falling_block();

				if (has_hanging_blocks()) {
					set_state_dropping_hanging();
				} else if (find_matches()) {
					set_state(STATE_FLARES);
				} else if (is_game_over()) {
					set_state(STATE_WAITING_CLIPS);
				} else {
					start_sound(SOUND_BLOCK_MISS, false);
					set_state_falling_block_or_hint();
				}
			}
			break;

		case STATE_DROPPING_HANGING:
			if (!update_dropping_blocks(dt)) {
				drop_hanging_blocks();

				assert(!has_hanging_blocks());

				if (find_matches()) {
					set_state(STATE_FLARES);
				} else {
					set_state(STATE_WAITING_CLIPS);
				}
			}
			break;

		case STATE_SOLVING_MATCHES:
			if (state_tics_ >= cur_settings.animation.solve_tics*MS_PER_TIC) {
				if (has_hanging_blocks()) {
					set_state_dropping_hanging();
				} else {
					set_state(STATE_WAITING_CLIPS);
				}
			}
			break;

		case STATE_FLARES:
			if (state_tics_ >= FLARE_TICS) {
				solve_matches();
				set_state(STATE_SOLVING_MATCHES);
			}
			break;

		case STATE_WAITING_CLIPS:
#if 0
			if (!clip_player.playing_clips()) {
				if (timer_display->finished()) {
					set_state_game_over(true);
				} else if (is_game_over()) {
					set_state_game_over(false);
				} else if (is_level_completed()) {
					finish_level();
				} else {
					set_state_falling_block_or_hint();
				}
			}
#else
			if (sprites_.empty()) {
				if (is_game_over()) {
					set_state(STATE_GAME_OVER);
				} else if (is_level_completed()) {
					set_state(STATE_LEVEL_COMPLETED);
				} else {
					set_state_falling_block_or_hint();
				}
			}
#endif
			break;

		case STATE_LEVEL_COMPLETED:
		case STATE_GAME_OVER:
			break;

		default:
			assert(0);
	}
}

void
world::draw() const
{
	render::set_blend_mode(blend_mode::ALPHA_BLEND);
#ifdef FIX_ME
	draw_background(mat);
#endif
	draw_blocks();

	if (cur_state_ == STATE_FLARES)
		draw_flares();

	for (const auto p : sprites_)
		p->draw();
}

void
world::draw_blocks() const
{
	for (int c = 0; c < cols_; c++) {
		const float x = c*cell_size_;

		for (int r = 0; r < rows_; r++) {
			const float y = r*cell_size_;

			if (int t = get_block_at(r, c)) {
				if (cur_state_ == STATE_HINT && r == hint_r_ && c == hint_c_) {
					// OMG HACK
					const float alpha = static_cast<hint_text_box *>(sprites_.front())->get_alpha();
					const g2d::rgb base_color = !(t & BAKUDAN_FLAG) ? theme_color_ : theme_opposite_color_;
					const g2d::rgb color = (1. - alpha)*base_color + alpha*g2d::rgb(255, 255, 255);
					draw_block(t - 1, x, y, 1, color);
				} else {
					draw_block(t - 1, x, y, 1);
				}
			} else {
				if (cur_state_ == STATE_DROPPING_HANGING)
					break;
			}
		}
	}

	if (cur_state_ == STATE_FALLING_BLOCK && CUR_FALLING_BLOCK->get_is_active())
		CUR_FALLING_BLOCK->draw();

	if (cur_state_ == STATE_DROPPING_HANGING) {
		for (int i = 0; i < num_dropping_blocks_; i++) {
			const auto& p = dropping_blocks_[i];
			float y = p.height_;
			draw_block(p.type_, p.col_*cell_size_, y, 1.);
		}
	}

#if 0
	block_sprite::draw_all(gv, lerp_dt);
#endif
}

void
world::draw_flares() const
{
	// make sure grid_find_matches was called before this!

	const float du = flare_texture_->get_u_scale()/FLARE_TEXTURE_ROWS;
	const float dv = flare_texture_->get_v_scale()/FLARE_TEXTURE_ROWS;

	int cur_frame = state_tics_*FLARE_NUM_FRAMES/FLARE_TICS;

	const float u0 = du*(cur_frame%FLARE_TEXTURE_ROWS);
	const float u1 = u0 + du;

	const float v0 = dv*(cur_frame/FLARE_TEXTURE_ROWS);
	const float v1 = v0 + dv;

	const float frame_size = FLARE_FRAME_SIZE;

	render::set_blend_mode(blend_mode::ADDITIVE_BLEND);
	render::set_color({ 1.f, 1.f, 1.f, 1.f });

	for (int i = 0; i < rows_*cols_; i++) {
		if (matches_[i] && !(grid_[i] & BAKUDAN_FLAG)) {
			const int r = i/cols_;
			const int c = i%cols_;

			const float x0 = (c + .5)*cell_size_ - .5*frame_size;
			const float x1 = x0 + frame_size;

			const float y0 = (r + .5)*cell_size_ - .5*frame_size;
			const float y1 = y0 + frame_size;

			render::draw_quad(
				flare_texture_,
				{ { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } },
				{ { u0, v0 }, { u1, v0 }, { u1, v1 }, { u0, v1 } },
				20);
		}
	}
}

int
world::get_col_height(int c) const
{
	for (int r = 0; r < rows_; r++) {
		if (!get_block_at(r, c))
			return r*cell_size_;
	}

	return rows_*cell_size_;
}

const wchar_t *
world::get_cur_falling_blocks() const
{
	static wchar_t buf[3];

	auto& p = falling_block_queue_[falling_block_index_];
	buf[0] = block_infos[p.block_types[0] & ~BAKUDAN_FLAG].kanji;
	buf[1] = block_infos[p.block_types[1] & ~BAKUDAN_FLAG].kanji;

	return buf;
}

bool
world::get_hint(hint& h) const
{
	bool found = false;

	int index = 1;

	for (int i = 0; i < 2; i++) {
		int block_index = falling_block_queue_[falling_block_index_].get_block_type(i);

		for (int r = 0; r < rows_; r++) {
			for (int c = 0; c < cols_; c++) {
				if (get_block_type_at(r, c))
					continue;

				if (!(r == 0 || get_block_type_at(r - 1, c) != 0))
					continue;

				int other;

				if (c < cols_ - 1 && (other = get_block_type_at(r, c + 1)) != 0) {
					const jukugo *p = match_map[block_index][other - 1];
					if (p) {
						if ((rand() % index) == 0) {
							h.block_type = block_index;

							h.block_r = r;
							h.block_c = c;

							h.match_r = r;
							h.match_c = c + 1;

							h.jukugo = p;

							found = true;
						}

						++index;
					}
				}

				if (c > 0 && (other = get_block_type_at(r, c - 1)) != 0) {
					const jukugo *p = match_map[other - 1][block_index];
					if (p) {
						if ((rand() % index) == 0) {
							h.block_type = block_index;

							h.block_r = r;
							h.block_c = c;

							h.match_r = r;
							h.match_c = c - 1;

							h.jukugo = p;
							found = true;
						}

						++index;
					}
				}

				if (r > 0) {
					other = get_block_type_at(r - 1, c);
					const jukugo *p = match_map[block_index][other - 1];
					if (p) {
						if ((rand() % index) == 0) {
							h.block_type = block_index;

							h.block_r = r;
							h.block_c = c;

							h.match_r = r - 1;
							h.match_c = c;

							h.jukugo = p;

							found = true;
						}

						++index;
					}
				}
			}
		}
	}

	return found;
}

bool
world::update_dropping_blocks(uint32_t dt)
{
	bool rv = false;

	for (int i = 0; i < num_dropping_blocks_; i++) {
		auto& p = dropping_blocks_[i];

		if (!p.active_)
			continue;

		rv = true;

		static const float DROP_GRAVITY = .8/(MS_PER_TIC*MS_PER_TIC);

		p.height_ += dt*p.speed_;
		p.speed_ -= dt*DROP_GRAVITY;

		float min_height = get_col_height(p.col_);

		for (int j = 0; j < i; j++) {
			const auto& q = dropping_blocks_[j];

			if (q.col_ == p.col_)
				min_height = std::max(min_height, q.height_ + cell_size_);
		}

		if (p.height_ <= min_height) {
			p.height_ = min_height;
			p.speed_ = .5*fabs(p.speed_);
		}

		static const float EPSILON = 2./MS_PER_TIC;

		if (fabs(p.speed_) < EPSILON && fabs(p.height_ - p.dest_height_) < EPSILON) {
			p.speed_ = 0;
			p.height_ = p.dest_height_;
			p.active_ = false;
		}
	}

	return rv;
}

void
world::drop_hanging_blocks()
{
	for (int c = 0; c < cols_; c++) {
		int last_empty = 0;

		for (int r = 0; r < rows_; r++) {
			if (get_block_at(r, c)) {
				if (r != last_empty) {
					set_block_at(last_empty, c, get_block_at(r, c));
					set_block_at(r, c, 0);
				}
				++last_empty;
			}
		}
	}
}

void
world::init_background_va()
{
	const block_info& bi = block_infos[NUM_BLOCK_TYPES];
	assert(bi.kanji == L'\0');

	const block_info::texuv& t = bi.texuvs[0];

	const float sv = blocks_texture_->get_u_scale();
	const float su = blocks_texture_->get_v_scale();

	const float u0 = su*t.u0;
	const float u1 = su*t.u1;

	const float v0 = sv*t.v0;
	const float v1 = sv*t.v1;

	for (int r = 0; r < rows_; r++) {
		const float y = r*cell_size_;

		for (int c = 0; c < cols_; c++) {
			const float x = c*cell_size_;

			background_va_ << x, y, v0, u1;
			background_va_ << x + cell_size_, y, v1, u1;
			background_va_ << x + cell_size_, y + cell_size_, v1, u0;
			background_va_ << x, y + cell_size_, v0, u0;

			const int vert_index = (r*cols_ + c)*4;
			background_va_ < vert_index + 0, vert_index + 1, vert_index + 2,
				         vert_index + 2, vert_index + 3, vert_index + 0;
		}
	}
}

void
world::draw_background(const g2d::mat4& mat) const
{
	program_grid_background& prog = get_program_instance<program_grid_background>();
	prog.use();
	prog.set_proj_modelview_matrix(mat);
	prog.set_texture(0);
	prog.set_theme_color((1./255)*.8*theme_color_);
	prog.set_highlight_position(g2d::vec2(-.1*cols_*cell_size_, 1.1*rows_*cell_size_));
	prog.set_highlight_fade_factor(.5f*cols_*cell_size_);

	background_va_.draw(GL_TRIANGLES);
}

void
world::draw_block(int type, float x, float y, float alpha) const
{
	draw_block(type, x, y, alpha, !(type & BAKUDAN_FLAG) ? theme_color_ : theme_opposite_color_);
}

void
world::draw_block(int type, float x, float y, float alpha, const g2d::rgb& color) const
{
	const block_info& bi = block_infos[type & ~BAKUDAN_FLAG];
	const block_info::texuv& t = bi.texuvs[!!(type & BAKUDAN_FLAG)];

	const float sv = blocks_texture_->get_u_scale();
	const float su = blocks_texture_->get_v_scale();

	const float u0 = su*t.u0;
	const float u1 = su*t.u1;

	const float v0 = sv*t.v0;
	const float v1 = sv*t.v1;

#if 0
	const int r = color.r;
	const int g = color.g;
	const int b = color.b;
	const int a = alpha*255;

	const int vert_index = gv.get_num_verts();

	gv << x, y, v0, u1, r, g, b, a;
	gv << x + cell_size_, y, v1, u1, r, g, b, a;
	gv << x + cell_size_, y + cell_size_, v1, u0, r, g, b, a;
	gv << x, y + cell_size_, v0, u0, r, g, b, a;

	gv < vert_index + 0, vert_index + 1, vert_index + 2,
	     vert_index + 2, vert_index + 3, vert_index + 0;
#else
	render::set_color({ color.r/255.f, color.g/255.f, color.b/255.f, alpha });
	render::draw_quad(
		blocks_texture_,
		{ { x, y }, { x + cell_size_, y }, { x + cell_size_, y + cell_size_ }, { x, y + cell_size_ } },
		{ { v0, u1 }, { v1, u1 }, { v1, u0 }, { v0, u0 } },
		0);
#endif
}

void
world::spawn_drop_trail(int row, int col, int type)
{
	const block_info& bi = block_infos[type & ~BAKUDAN_FLAG];
	const block_info::texuv& t = bi.texuvs[!!(type & BAKUDAN_FLAG)];

	const float sv = blocks_texture_->get_u_scale();
	const float su = blocks_texture_->get_v_scale();

	g2d::vec2 uv0(su*t.u0, sv*t.v0);
	g2d::vec2 uv1(su*t.u1, sv*t.v1);

	sprites_.push_front(
		new drop_trail_sprite(
			cell_size_*g2d::vec2(col, row),
			cell_size_,
			blocks_texture_,
			uv0, uv1, 
			!(type & BAKUDAN_FLAG) ? theme_color_ : theme_opposite_color_));
}

void
world::spawn_dead_block_sprite(const g2d::vec2& pos, int type)
{
	const block_info& bi = block_infos[type & ~BAKUDAN_FLAG];
	const block_info::texuv& t = bi.texuvs[!!(type & BAKUDAN_FLAG)];

	const float sv = blocks_texture_->get_u_scale();
	const float su = blocks_texture_->get_v_scale();

	g2d::vec2 uv0(su*t.u0, sv*t.v0);
	g2d::vec2 uv1(su*t.u1, sv*t.v1);

	sprites_.push_front(
		new dead_block_sprite(
			pos,
			cell_size_,
			blocks_texture_,
			uv0, uv1,
			!(type & BAKUDAN_FLAG) ? theme_color_ : theme_opposite_color_));
}
