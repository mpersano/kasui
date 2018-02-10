#include <algorithm>
#include <vector>
#include <map>

#include "program.h"
#include "texture.h"
#include "vertex_array.h"
#include "font.h"
#include "xwchar.h"
#include "draw_queue.h"

namespace g2d {

class draw_queue_impl;

class draw_queue_entry
{
public:
	virtual ~draw_queue_entry() { }

	virtual void execute(const draw_queue_impl&) const = 0;
};

class draw_queue_impl
{
	enum text_align { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

public:
	draw_queue_impl();
	~draw_queue_impl();

	void enable_texture(const texture *tex);
	void disable_texture();

	void use_program(const program *prog);

	void set_matrix(const mat3& matrix);
	void translate(const vec2& pos);
	void translate(float x, float y);
	void scale(const float s);
	void rotate(float a);
	void push_matrix();
	void pop_matrix();

	void align_left();
	void align_right();
	void align_center();

	void enable_text_outline();
	void disable_text_outline();

	void text_program(const program *prog);
	void text_outline_program(const program *prog);

	void render_text(const font *f, const wchar_t *fmt, va_list ap);

	void add(draw_queue_entry *entry);

	void draw() const;

	void reset();

private:
	void render_chars(const font *f, float x, float y, const wchar_t *p);

	struct matrix_stack {
		matrix_stack()
		: size(0) { }

		void push(const mat3& m)
		{ assert(size < MAX_SIZE); stack[size++] = m; }

		mat3 pop()
		{ assert(size > 0); return stack[--size]; }

		void reset()
		{ size = 0; }

		int size;
		enum { MAX_SIZE = 16 };
		mat3 stack[MAX_SIZE];
	};

	mat3 cur_matrix_;
	matrix_stack matrix_stack_;

	text_align text_align_;
	bool text_outline_;

	const program *text_program_;
	const program *text_outline_program_;

	std::vector<draw_queue_entry *> entries_;
};

//
//  draw_queue_entry
//

class enable_texture_entry : public draw_queue_entry
{
public:
	enable_texture_entry(const texture *tex)
	: texture_(tex)
	{ }

	void execute(const draw_queue_impl&) const override
	{
		glEnable(GL_TEXTURE_2D);
		texture_->bind();
	}

private:
	const texture *texture_;
};

class disable_texture_entry : public draw_queue_entry
{
public:
	void execute(const draw_queue_impl&) const override
	{ glDisable(GL_TEXTURE_2D); }
};

class use_program_entry : public draw_queue_entry
{
public:
	use_program_entry(const program *prog)
	: program_(prog)
	{ }

	void execute(const draw_queue_impl&) const override
	{ program_->use(); }

private:
	const program *program_;
};

template <typename VertexArray>
class vertex_array_entry : public draw_queue_entry
{
public:
	vertex_array_entry(GLenum mode)
	: mode_(mode)
	{ }

	void execute(const draw_queue_impl&) const override
	{ vertex_array_.draw(mode_); }

	VertexArray& vertex_array()
	{ return vertex_array_; }

private:
	VertexArray vertex_array_;
	GLenum mode_;
};


//
//  draw_queue_impl
//

draw_queue_impl::draw_queue_impl()
: cur_matrix_(mat3::identity())
, text_align_(ALIGN_LEFT)
, text_outline_(false)
, text_program_(nullptr)
, text_outline_program_(nullptr)
{ }

draw_queue_impl::~draw_queue_impl()
{
	std::for_each(
		entries_.begin(),
		entries_.end(),
		[](draw_queue_entry *e) { delete e; });
}

void
draw_queue_impl::enable_texture(const texture *tex)
{
	add(new enable_texture_entry(tex));
}

void
draw_queue_impl::disable_texture()
{
	add(new disable_texture_entry);
}

void
draw_queue_impl::use_program(const program *prog)
{
	add(new use_program_entry(prog));
}

void
draw_queue_impl::set_matrix(const mat3& mat)
{
	cur_matrix_ = mat;
}

void
draw_queue_impl::translate(const vec2& pos)
{
	translate(pos.x, pos.y);
}

void
draw_queue_impl::translate(float x, float y)
{
	cur_matrix_ *= mat3::translation(x, y);
}

void
draw_queue_impl::scale(float s)
{
	cur_matrix_ *= mat3::scale(s);
}

void
draw_queue_impl::rotate(float a)
{
	cur_matrix_ *= mat3::rotation(a);
}

void
draw_queue_impl::push_matrix()
{
	matrix_stack_.push(cur_matrix_);
}

void
draw_queue_impl::pop_matrix()
{
	cur_matrix_ = matrix_stack_.pop();
}

void
draw_queue_impl::align_left()
{
	text_align_ = ALIGN_LEFT;
}

void
draw_queue_impl::align_right()
{
	text_align_ = ALIGN_RIGHT;
}

void
draw_queue_impl::align_center()
{
	text_align_ = ALIGN_CENTER;
}

void
draw_queue_impl::enable_text_outline()
{
	text_outline_ = true;
}

void
draw_queue_impl::disable_text_outline()
{
	text_outline_ = false;
}

void
draw_queue_impl::text_program(const program *prog)
{
	text_program_ = prog;
}

void
draw_queue_impl::text_outline_program(const program *prog)
{
	text_outline_program_ = prog;
}

void
draw_queue_impl::render_text(const font *f, const wchar_t *fmt, va_list ap)
{
	static wchar_t buf[512];
	xvswprintf(buf, fmt, ap);

	float x_start;

	switch (text_align_) {
		case ALIGN_RIGHT:
			x_start = -f->get_string_width(buf);
			break;

		case ALIGN_CENTER:
			x_start = -.5*f->get_string_width(buf);
			break;

		default:
			x_start = 0;
			break;
	}

	assert(text_program_);

	if (!text_outline_) {
		// no outline, just render this thing
		use_program(text_program_);
		render_chars(f, x_start, 0, buf);
	} else {
		assert(text_outline_program_);

		// render outline
		use_program(text_outline_program_);
		render_chars(f, x_start, 0, buf);

		// render interior
		use_program(text_program_);
		render_chars(f, x_start, 0, buf);
	}
}

void
draw_queue_impl::render_chars(const font *f, float x, float y, const wchar_t *str)
{
	using namespace vertex;

	typedef indexed_vertex_array<GLubyte, attrib<GLfloat, 2>, attrib<GLfloat, 2> > vertex_array_type;

	vertex_array_entry<vertex_array_type> *entry = new vertex_array_entry<vertex_array_type>(GL_TRIANGLES);

	vertex_array_type& va = entry->vertex_array();

	int vert_index = 0;

	for (const wchar_t *p = str; *p; p++) {
		const glyph_info *g = f->find_glyph(*p);

		const vec2& t0 = g->texuv[0];
		const vec2& t1 = g->texuv[1];
		const vec2& t2 = g->texuv[2];
		const vec2& t3 = g->texuv[3];

		const float x_left = x + g->left;
		const float x_right = x_left + g->width;
		const float y_top = y + g->top;
		const float y_bottom = y_top - g->height;

		const vec2 p0 = cur_matrix_*vec2(x_left, y_top);
		const vec2 p1 = cur_matrix_*vec2(x_right, y_top);
		const vec2 p2 = cur_matrix_*vec2(x_right, y_bottom);
		const vec2 p3 = cur_matrix_*vec2(x_left, y_bottom);

		va << p0.x, p0.y, t0.x, t0.y;
		va << p1.x, p1.y, t1.x, t1.y;
		va << p2.x, p2.y, t2.x, t2.y;
		va << p3.x, p3.y, t3.x, t3.y;

		va < vert_index + 0, vert_index + 1, vert_index + 2;
		va < vert_index + 2, vert_index + 3, vert_index + 0;

		x += g->advance_x;
		vert_index += 4;
	}

	enable_texture(f->get_texture());
	add(entry);
}

void
draw_queue_impl::add(draw_queue_entry *entry)
{
	entries_.push_back(entry);
}

void
draw_queue_impl::draw() const
{
	std::for_each(
		entries_.begin(),
		entries_.end(),
		[this](const draw_queue_entry *e) { e->execute(*this); });
}

void
draw_queue_impl::reset()
{
	std::for_each(
		entries_.begin(),
		entries_.end(),
		[](draw_queue_entry *e) { delete e; });
	entries_.clear();

	cur_matrix_ = mat3::identity();
	matrix_stack_.reset();
}

draw_queue::draw_queue()
: impl_(new draw_queue_impl)
{ }

draw_queue::~draw_queue()
{
	delete impl_;
}

draw_queue&
draw_queue::set_matrix(const mat3& mat)
{
	impl_->set_matrix(mat);
	return *this;
}

draw_queue &
draw_queue::translate(const vec2& pos)
{
	impl_->translate(pos);
	return *this;
}

draw_queue&
draw_queue::translate(float x, float y)
{
	impl_->translate(x, y);
	return *this;
}

draw_queue&
draw_queue::scale(float s)
{
	impl_->scale(s);
	return *this;
}

draw_queue&
draw_queue::rotate(float a)
{
	impl_->rotate(a);
	return *this;
}

draw_queue&
draw_queue::push_matrix()
{
	impl_->push_matrix();
	return *this;
}

draw_queue&
draw_queue::pop_matrix()
{
	impl_->pop_matrix();
	return *this;
}

draw_queue&
draw_queue::align_left()
{
	impl_->align_left();
	return *this;
}

draw_queue&
draw_queue::align_right()
{
	impl_->align_right();
	return *this;
}

draw_queue&
draw_queue::align_center()
{
	impl_->align_center();
	return *this;
}

draw_queue&
draw_queue::enable_text_outline()
{
	impl_->enable_text_outline();
	return *this;
}

draw_queue&
draw_queue::disable_text_outline()
{
	impl_->disable_text_outline();
	return *this;
}

draw_queue&
draw_queue::text_program(const program *prog)
{
	impl_->text_program(prog);
	return *this;
}

draw_queue&
draw_queue::text_outline_program(const program *prog)
{
	impl_->text_outline_program(prog);
	return *this;
}

draw_queue&
draw_queue::render_text(const font *f, const wchar_t *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	impl_->render_text(f, fmt, ap);
	va_end(ap);

	return *this;
}

void
draw_queue::draw() const
{
	impl_->draw();
}

void
draw_queue::reset()
{
	impl_->reset();
}

}
