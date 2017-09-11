#include "falling_leaves_theme.h"
#include "flowers_theme.h"
#include "clouds_theme.h"

theme *themes[NUM_THEMES] = {
	&clouds_theme,
	&falling_leaves_theme,
	&flowers_theme,
};

void
themes_initialize()
{
	for (unsigned i = 0; i < NUM_THEMES; i++)
		themes[i]->initialize();
}
