#ifndef KASUI_LOGO_H_
#define KASUI_LOGO_H_

#include "title_widget.h"

namespace g2d {
class sprite;
}

class abstract_action;

struct kasui_logo : public title_widget
{
	kasui_logo();

	void reset();
	void update(uint32_t dt) override;
	void draw() const override;

	const g2d::sprite *bg, *ka, *sui;
	float ka_scale, sui_scale;
	float ka_mix, sui_mix;

	abstract_action *action;
};

#endif // KASUI_LOGO_H_
