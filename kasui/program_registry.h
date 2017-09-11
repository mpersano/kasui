#ifndef PROGRAM_REGISTRY_H_
#define PROGRAM_REGISTRY_H_

#include "singleton_registry.h"
#include "program_wrapper.h"

typedef singleton::registry<
	program_flat,
	program_color,
	program_texture_decal,
	program_texture_color,
	program_texture_alpha,
	program_texture_uniform_alpha,
	program_texture_uniform_color,
	program_text,
	program_text_outline,
	program_text_alpha,
	program_text_outline_alpha,
	program_text_gradient,
	program_text_outline_gradient,
	program_intro_text,
	program_timer_text,
	program_3d_texture_color,
	program_grid_background
	> program_registry;

template <typename T>
inline T&
get_program_instance()
{
	return singleton::find<T, program_registry>::result::get_instance();
}

template <typename T>
struct reload
{
	static void eval()
	{
		T::get_instance().reload();
		reload<typename T::base_type>::eval();
	}
};

template <>
struct reload<singleton::registry<> >
{
	static void eval()
	{ }
};

inline void
reload_all_programs()
{
	reload<program_registry>::eval();
}

#endif // PROGRAM_REGISTRY_H_
