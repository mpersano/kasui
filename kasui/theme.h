#ifndef THEME_H_
#define THEME_H_

#include <cstdint>

#include "settings.h"

struct theme {
	color_scheme *colors;
	void (*initialize)();
	void (*reset)();
	void (*draw)();
	void (*update)(uint32_t dt);
};

enum {
	THEME_CLOUDS,
	THEME_FALLING_LEAVES,
	THEME_FLOWERS,
	NUM_THEMES,
};

extern theme *themes[NUM_THEMES];

void
themes_initialize();

#endif // THEME_H_
