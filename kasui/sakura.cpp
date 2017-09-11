#include "guava2d/texture_manager.h"

#include "program_registry.h"
#include "common.h"
#include "sakura.h"

enum {
	FADE_TTL = 30*MS_PER_TIC,
};

static const int MIN_TTL = 150*MS_PER_TIC;
static const int MAX_TTL = 300*MS_PER_TIC;

static const float MIN_SIZE = 40;
static const float MAX_SIZE = 60;

static const float MIN_DELTA_ANGLE = .01;
static const float MAX_DELTA_ANGLE = .05;

static const float SPEED_FUZZ = .4;

static const float MIN_PHI = .025, MAX_PHI = .05;
static const float MIN_PHASE = 0, MAX_PHASE = M_PI;
static const float MIN_RADIUS = 30, MAX_RADIUS = 60;

static const float MIN_SPEED = 2.;
static const float MAX_SPEED = 4.;

static const float MIN_ALPHA = .3;
static const float MAX_ALPHA = .6;

void
sakura_petal::reset(bool from_start)
{
	size = frand(MIN_SIZE, MAX_SIZE);

	pos.x = frand(.5*window_width, 1.5*window_width);
	pos.y = window_height + size;

	const float f = 1.f/MS_PER_TIC;

	dir = g2d::vec2(-1, -2) + g2d::vec2(frand(-SPEED_FUZZ, SPEED_FUZZ), frand(-SPEED_FUZZ, SPEED_FUZZ));
	dir.set_length(f*frand(MIN_SPEED, MAX_SPEED));

	angle = 0;
	delta_angle = f*frand(MIN_DELTA_ANGLE, MAX_DELTA_ANGLE);

	phi_0 = f*frand(MIN_PHI, MAX_PHI);
	phi_1 = 2*phi_0;

	phase_0 = frand(MIN_PHASE, MAX_PHASE);
	phase_1 = frand(MIN_PHASE, MAX_PHASE);

	radius_0 = frand(MIN_RADIUS, MAX_RADIUS);
	radius_1 = .5*radius_0;

	alpha = frand(MIN_ALPHA, MAX_ALPHA);

	ttl = frand(MIN_TTL, MAX_TTL);

	if (from_start) {
		tics = 0;
	} else {
		tics = frand(0, ttl);
		pos += tics*dir;
		angle += tics*delta_angle;
	}
}

void
sakura_petal::draw(vertex_array_type& gv) const
{
	float a = cosf(phi_0*tics + phase_0);
	float b = cosf(phi_1*tics + phase_1);

	const g2d::vec2 p = pos + g2d::vec2(a*radius_0, b*radius_1);

	const float s = sinf(angle);
	const float c = cosf(angle);

	const g2d::vec2 up = g2d::vec2(s, c)*.5*size;
	const g2d::vec2 right = g2d::vec2(-c, s)*.5*size;

	const g2d::vec2 p0 = p + up - right;
	const g2d::vec2 p1 = p + up + right;
	const g2d::vec2 p2 = p - up + right;
	const g2d::vec2 p3 = p - up - right;

	float w;
	if (tics > ttl - FADE_TTL)
		w = 1. - static_cast<float>(tics - (ttl - FADE_TTL))/FADE_TTL;
	else
		w = 1;

	const int vert_index = gv.get_num_verts();

	{
	const float a = w*alpha;
	gv << p0.x, p0.y, 0, 0, a;
	gv << p1.x, p1.y, 1, 0, a;
	gv << p2.x, p2.y, 1, 1, a;
	gv << p3.x, p3.y, 0, 1, a;
	}

	gv < vert_index + 0, vert_index + 1, vert_index + 2;
	gv < vert_index + 2, vert_index + 3, vert_index + 0;
}

void
sakura_petal::update(uint32_t dt)
{
	pos += dt*dir;
	angle += dt*delta_angle;

	if ((tics += dt) >= ttl)
		reset(true);
}

sakura_fubuki::sakura_fubuki()
: petal_texture(g2d::texture_manager::get_instance().load("images/petal.png"))
{ }

void
sakura_fubuki::update(uint32_t dt)
{
	for (sakura_petal *p = petals; p != &petals[NUM_PETALS]; p++)
		p->update(dt);
}

void
sakura_fubuki::reset()
{
	for (sakura_petal *p = petals; p != &petals[NUM_PETALS]; p++)
		p->reset(false);
}

void
sakura_fubuki::draw(const g2d::mat4& proj_modelview) const
{
	static sakura_petal::vertex_array_type gv(NUM_PETALS*4, NUM_PETALS*6);

	gv.reset();

	for (const sakura_petal *p = petals; p != &petals[NUM_PETALS]; p++)
		p->draw(gv);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	petal_texture->bind();

	program_texture_alpha& prog = get_program_instance<program_texture_alpha>();
	prog.use();
	prog.set_proj_modelview_matrix(proj_modelview);
	prog.set_texture(0);

	gv.draw(GL_TRIANGLES);
}
