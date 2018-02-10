#include <stdlib.h>

#include "guava2d/g2dgl.h"
#include "guava2d/vertex_array.h"
#include "guava2d/texture_manager.h"
#include "guava2d/vec3.h"
#include "guava2d/rgb.h"

#include "common.h"
#include "program_registry.h"
#include "falling_leaves_theme.h"

enum {
	NUM_LEAVES = 30,
	FADE_TTL = 30*MS_PER_TIC,
};

static const float FOV = 45.f;
static const float Z_NEAR = 1.f;
static const float Z_FAR = 5000.f;

static const g2d::texture *leaf_texture;

using namespace g2d::vertex;

typedef g2d::indexed_vertex_array<
		GLubyte,
		attrib<GLfloat, 3>,
		attrib<GLshort, 2>,
		attrib<GLubyte, 4> > vertex_array;

static void
initialize_perspective_matrix(GLfloat matrix[16], float fovy, float aspect, float close, float phar) // far/near are defined on windoze!
{
	const float fovy_rad = fovy*M_PI/180.;

	const float f = 1./tan(.5*fovy_rad);

	matrix[0] = f/aspect; matrix[4] = 0; matrix[8] = 0; matrix[12] = 0;
	matrix[1] = 0; matrix[5] = f; matrix[9] = 0; matrix[13] = 0;
	matrix[2] = 0; matrix[6] = 0; matrix[10] = (phar + close)/(close - phar); matrix[14] = (2.*phar*close)/(close - phar);
	matrix[3] = 0; matrix[7] = 0; matrix[11] = -1; matrix[15] = 0;
}


struct leaf
{
	float size;
	g2d::vec3 pos, speed;
	g2d::rgb color;
	g2d::mat4 dir;
	g2d::vec3 axis;
	float rot;
	float phase_0, phase_1, phi_0, phi_1;
	float radius_0, radius_1;
	int tics, ttl;

	void reset();
	void draw(vertex_array& gv) const;
	void update(uint32_t dt);
};

static leaf leaves[NUM_LEAVES];

void
leaf::reset()
{
	const float f = 1.f/MS_PER_TIC;

	static const float MIN_X = -150, MAX_X = 150;
	static const float MIN_Y = 150, MAX_Y = 300;
	static const float MIN_Z = -700, MAX_Z = -200;

	static const float MIN_THETA = .03, MAX_THETA = .08;
	static const float SPEED_FUZZ = .1;
	static const float MIN_SPEED = 1, MAX_SPEED = 1.5;
	static const float MIN_PHI = .025, MAX_PHI = .05;
	static const float MIN_PHASE = 0, MAX_PHASE = M_PI;
	static const float MIN_RADIUS = 15, MAX_RADIUS = 30;
	static const int MIN_TTL = 200, MAX_TTL = 250;

	static const g2d::rgb min_color(255, 0, 0), max_color(255, 255, 0);

	size = 30;

	pos = g2d::vec3(frand(MIN_X, MAX_X), frand(MIN_Y, MAX_Y), frand(MIN_Z, MAX_Z));

	axis = g2d::vec3(frand() - .5, frand() - .5, frand() - .5).normalize();
	rot = f*frand(MIN_THETA, MAX_THETA);

	dir = g2d::mat4::identity();

	speed = g2d::vec3(frand(-SPEED_FUZZ, SPEED_FUZZ),
	  -1, frand(SPEED_FUZZ, SPEED_FUZZ)).normalize()*frand(MIN_SPEED, MAX_SPEED)*f;

	phi_0 = f*frand(MIN_PHI, MAX_PHI);
	phi_1 = 2*phi_0;

	phase_0 = frand(MIN_PHASE, MAX_PHASE);
	phase_1 = frand(MIN_PHASE, MAX_PHASE);

	radius_0 = frand(MIN_RADIUS, MAX_RADIUS);
	radius_1 = .5*radius_0;

	ttl = (MIN_TTL + rand()%(MAX_TTL - MIN_TTL))*MS_PER_TIC;

	color = min_color + (max_color - min_color)*frand();

	tics = 0;
}

void
leaf::draw(vertex_array& gv) const
{
	float alpha_scale;

	if (tics < FADE_TTL)
		alpha_scale = static_cast<float>(tics)/FADE_TTL;
	else if (tics > ttl - FADE_TTL)
		alpha_scale = 1. - static_cast<float>(tics - (ttl - FADE_TTL))/FADE_TTL;
	else
		alpha_scale = 1;

	float s0 = cosf(phi_0*tics + phase_0);
	float s1 = cosf(phi_1*tics + phase_1);

	const g2d::vec3 o = pos;

	const g2d::vec3 p = g2d::vec3(o.x + s0*radius_0, o.y + s1*radius_1, o.z);

	const g2d::vec3 p0 = p + dir*g2d::vec3(-size, -size, 0);
	const g2d::vec3 p1 = p + dir*g2d::vec3(size, -size, 0);
	const g2d::vec3 p2 = p + dir*g2d::vec3(size, size, 0);
	const g2d::vec3 p3 = p + dir*g2d::vec3(-size, size, 0);

	const int r = color.r;
	const int g = color.g;
	const int b = color.b;
	const int a = alpha_scale*255;

	const int vert_index = gv.get_num_verts();

	gv << p0.x, p0.y, p0.z, 0, 0, r, g, b, a;
	gv << p1.x, p1.y, p1.z, 1, 0, r, g, b, a;
	gv << p2.x, p2.y, p2.z, 1, 1, r, g, b, a;
	gv << p3.x, p3.y, p3.z, 0, 1, r, g, b, a;

	gv < vert_index + 0, vert_index + 1, vert_index + 2,
	     vert_index + 2, vert_index + 3, vert_index + 0;
}

void
leaf::update(uint32_t dt)
{
	g2d::mat4 r = g2d::mat4::rotation_from_axis_and_angle(axis, dt*rot);
	dir *= r;
	pos += dt*speed;

	if ((tics += dt) >= ttl)
		reset();
}

static void
initialize()
{
	leaf_texture = g2d::texture_manager::get_instance().load("images/leaf.png");
}

static void
reset()
{
	for (leaf *p = leaves; p != &leaves[NUM_LEAVES]; p++)
		p->reset();
}

static void
update(uint32_t dt)
{
	for (leaf *p = leaves; p != &leaves[NUM_LEAVES]; p++)
		p->update(dt);
}

static int
leaf_z_compare(const void *foo, const void *bar)
{
	const float z0 = (*(leaf **)foo)->pos.z;
	const float z1 = (*(leaf **)bar)->pos.z;
	return z0 < z1 ? -1 : 1;
}

static void
draw()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	leaf_texture->bind();

	static const leaf *sorted_leaves[NUM_LEAVES];

	for (int i = 0; i < NUM_LEAVES; i++)
		sorted_leaves[i] = &leaves[i];

	qsort(sorted_leaves, NUM_LEAVES, sizeof *sorted_leaves, leaf_z_compare);

	static vertex_array gv(NUM_LEAVES*4, NUM_LEAVES*6);

	gv.reset();

	for (int i = 0; i < NUM_LEAVES; i++)
		sorted_leaves[i]->draw(gv);

	GLfloat matrix[16];
	initialize_perspective_matrix(matrix, FOV, window_width/window_height, Z_NEAR, Z_FAR);

	program_3d_texture_color& prog = get_program_instance<program_3d_texture_color>();
	prog.use();
	prog.set_proj_modelview_matrix(matrix);
	prog.set_texture(0);

	gv.draw(GL_TRIANGLES);
}

theme falling_leaves_theme = {
	nullptr,
	initialize,
	reset,
	draw,
	update
};
