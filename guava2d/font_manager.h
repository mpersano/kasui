#ifndef FONT_MANAGER_H_
#define FONT_MANAGER_H_

#include <map>
#include <string>

#include "font.h"

namespace g2d {

class font_manager
{
	typedef std::map<std::string, font *> dict_type;
	typedef typename dict_type::value_type dict_value_type;

public:
	static font_manager& get_instance();

	const font *load(const char *source);

private:
	font_manager();

	dict_type font_dict_;
};

}

#endif // FONT_MANAGER_H_
