#ifndef TEXTURE_MANAGER_H_
#define TEXTURE_MANAGER_H_

#include <map>
#include <string>

#include "texture.h"

namespace g2d {

class texture_manager
{
	typedef std::map<std::string, texture *> dict_type;
	typedef typename dict_type::value_type dict_value_type;

public:
	static texture_manager& get_instance();

	void set_downsample_scale(int scale);

	const texture *load(const char *source);
	void put(const char *name, texture *t);

	void load_all();

private:
	texture_manager();

	dict_type texture_dict_;
	int downsample_scale_;
};

}

#endif // TEXTURE_MANAGER_H_
