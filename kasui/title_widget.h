#ifndef TITLE_WIDGET_H_
#define TITLE_WIDGET_H_

#include "common.h"

namespace g2d {
class mat4;
};

struct title_widget {
	title_widget();
	virtual ~title_widget() { }

	virtual void update(uint32_t dt);
	virtual void reset();
	virtual void draw(const g2d::mat4& proj_modelview) const = 0;

	enum state {
		OUTSIDE,
		ENTERING,
		INSIDE,
		LEAVING
	};

	void set_state(state next_state);
	void show();
	void hide();

	enum {
		ENTER_T = 30*MS_PER_TIC,
		LEAVE_T = 20*MS_PER_TIC,
	};

	state cur_state;
	uint32_t state_t;
};

#endif // TITLE_WIDGET_H_
