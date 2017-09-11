#ifndef BACKGROUND_H_
#define BACKGROUND_H_

namespace g2d {
class mat4;
class rgb;
};

void
background_draw_gradient();

void
background_initialize_gradient(const g2d::rgb& from_color, const g2d::rgb& to_color);

void
background_initialize();

#endif // BACKGROUND_H_
