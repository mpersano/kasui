#ifndef G2D_VERTEX_ARRAY_H_
#define G2D_VERTEX_ARRAY_H_

#include <cassert>
#include <vector>

#include "g2dgl.h"
#include "vec2.h"

namespace g2d {

namespace vertex {

template <typename T, int N>
struct attrib
{
	typedef T value_type;
	static const int size = N;

	value_type value_[size];
};

template <typename... Attrs>
struct vertex;

template <typename Head, typename... Tail>
struct vertex<Head, Tail...> : vertex<Tail...>
{
	typedef vertex<Tail...> base_type;
	typedef Head attr_type;
	typedef typename Head::value_type attr_value_type;

	attr_type attr_;
};

template <>
struct vertex<>
{ };

template <typename GLType>
struct gltype_to_glenum;

template <>
struct gltype_to_glenum<GLbyte>
{
	static const GLenum type = GL_BYTE;
};

template <>
struct gltype_to_glenum<GLubyte>
{
	static const GLenum type = GL_UNSIGNED_BYTE;
};

template <>
struct gltype_to_glenum<GLshort>
{
	static const GLenum type = GL_SHORT;
};

template <>
struct gltype_to_glenum<GLushort>
{
	static const GLenum type = GL_UNSIGNED_SHORT;
};

template <>
struct gltype_to_glenum<GLint>
{
	static const GLenum type = GL_INT;
};

template <>
struct gltype_to_glenum<GLuint>
{
	static const GLenum type = GL_UNSIGNED_INT;
};

template <>
struct gltype_to_glenum<GLfloat>
{
	static const GLenum type = GL_FLOAT;
};

template <typename Vertex>
inline void
enable_client_state(const Vertex& v, const GLsizei attrib_index, size_t vertex_size)
{
	glVertexAttribPointer(attrib_index, Vertex::attr_type::size, gltype_to_glenum<typename Vertex::attr_type::value_type>::type, GL_FALSE, vertex_size, &v.attr_);
	glEnableVertexAttribArray(attrib_index);

	enable_client_state(static_cast<const typename Vertex::base_type&>(v), attrib_index + 1, vertex_size);
}

inline void
enable_client_state(const vertex<>&, const GLsizei, size_t)
{ }

template <typename Vertex>
inline void
disable_client_state(const Vertex& v, const GLsizei attrib_index)
{
	disable_client_state(static_cast<const typename Vertex::base_type&>(v), attrib_index + 1);

	glDisableVertexAttribArray(attrib_index);
}

inline void
disable_client_state(const vertex<>&, const GLsizei)
{ }

template <int N, typename Vertex>
struct comma_initializer
{
	comma_initializer(Vertex& vertex)
	: vertex_(vertex)
	{ }

	comma_initializer<N + 1, Vertex>
	operator,(typename Vertex::attr_value_type v)
	{
		vertex_.attr_.value_[N] = v;
		return comma_initializer<N + 1, Vertex>(vertex_);
	}

	Vertex& vertex_;
};

template <typename Vertex>
struct comma_initializer<Vertex::attr_type::size - 1, Vertex>
{
	comma_initializer(Vertex& vertex)
	: vertex_(vertex)
	{ }

	comma_initializer<0, typename Vertex::base_type>
	operator,(typename Vertex::attr_value_type v)
	{
		vertex_.attr_.value_[Vertex::attr_type::size - 1] = v;
		return comma_initializer<0, typename Vertex::base_type>(static_cast<typename Vertex::base_type&>(vertex_));
	}

	Vertex& vertex_;
};

template <typename Attrib>
struct comma_initializer<vertex<Attrib>::attr_type::size - 1, vertex<Attrib> >
{
	comma_initializer(vertex<Attrib>& vertex)
	: vertex_(vertex)
	{ }

	void
	operator,(typename Attrib::value_type v)
	{
		vertex_.attr_.value_[Attrib::size - 1] = v;
	}

	vertex<Attrib>& vertex_;
};

}

template <typename Impl, typename... VertexAttrs>
class vertex_array_base
{
public:
	typedef vertex::vertex<VertexAttrs...> vertex_type;

	vertex_array_base()
	{ }

	vertex_array_base(int max_verts)
	{ verts_.reserve(max_verts); }

	void reset()
	{ verts_.clear(); }

	vertex::comma_initializer<1, vertex_type>
	operator<<(const typename vertex_type::attr_value_type& v)
	{
		verts_.resize(verts_.size() + 1);

		vertex_type& vertex = verts_.back();
		vertex.attr_.value_[0] = v;
		return vertex::comma_initializer<1, vertex_type>(vertex);
	}

	void draw(GLenum mode) const
	{
		using namespace vertex;

		enable_client_state(verts_[0], 0, sizeof(vertex_type));
		static_cast<const Impl *>(this)->gl_draw(mode);
		disable_client_state(verts_[0], 0);
	}

	int get_num_verts() const
	{ return verts_.size(); }

protected:
	std::vector<vertex_type> verts_;
};

template <typename... VertexAttrs>
class vertex_array : public vertex_array_base<vertex_array<VertexAttrs...>, VertexAttrs...>
{
public:
	vertex_array()
	{ }

	vertex_array(int max_verts)
	: vertex_array_base<vertex_array, VertexAttrs...>(max_verts)
	{ }

	void gl_draw(GLenum mode) const
	{
		glDrawArrays(mode, 0, this->verts_.size());
	}
};

template <typename IndexType, typename... VertexAttrs>
class indexed_vertex_array : public vertex_array_base<indexed_vertex_array<IndexType, VertexAttrs...>, VertexAttrs...>
{
public:
	typedef IndexType index_type;

	indexed_vertex_array()
	{ }

	indexed_vertex_array(int max_verts, int max_indices)
	: vertex_array_base<indexed_vertex_array, VertexAttrs...>(max_verts)
	{
		indices_.reserve(max_indices);
	}

	void reset()
	{
		vertex_array_base<indexed_vertex_array, VertexAttrs...>::reset();
		indices_.clear();
	}

	struct index_comma_initializer
	{
		index_comma_initializer(indexed_vertex_array& parent)
		: parent_(parent)
		{ }

		index_comma_initializer&
		operator,(const index_type& index)
		{
			parent_.add_index(index);
			return *this;
		}

		indexed_vertex_array& parent_;
	};

	index_comma_initializer
	operator<(const index_type& index)
	{
		add_index(index);
		return index_comma_initializer(*this);
	}

	void add_index(const index_type& index)
	{
		indices_.push_back(index);
	}

	void gl_draw(GLenum mode) const
	{
		using namespace vertex;
		glDrawElements(mode, indices_.size(), gltype_to_glenum<IndexType>::type, &indices_[0]);
	}

	int get_num_indices() const
	{
		return indices_.size();
	}

protected:
	std::vector<index_type> indices_;
};

typedef vertex_array<
		vertex::attrib<GLfloat, 2> > vertex_array_flat;

typedef vertex_array<
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLubyte, 4> > vertex_array_color;

typedef vertex_array<
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLfloat, 2> > vertex_array_texuv;

typedef vertex_array<
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLubyte, 4> > vertex_array_texuv_color;

typedef vertex_array<
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLfloat, 1> > vertex_array_texuv_alpha;

typedef indexed_vertex_array<
		GLushort,
		vertex::attrib<GLfloat, 2> > indexed_vertex_array_flat;

typedef indexed_vertex_array<
		GLushort,
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLubyte, 4> > indexed_vertex_array_color;

typedef indexed_vertex_array<
		GLushort,
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLfloat, 2> > indexed_vertex_array_texuv;

typedef indexed_vertex_array<
		GLushort,
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLubyte, 4> > indexed_vertex_array_texuv_color;

typedef indexed_vertex_array<
		GLushort,
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLfloat, 2>,
		vertex::attrib<GLfloat, 1> > indexed_vertex_array_texuv_alpha;

}

#endif // G2D_VERTEX_ARRAY_H_
