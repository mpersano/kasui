#ifndef G2D_GL_H_
#define G2D_GL_H_

#ifdef ANDROID_NDK
#include <GLES3/gl3.h>
#define glOrtho glOrthof
void gluPerspective(GLfloat, GLfloat, GLfloat, GLfloat);
#else
#include <GL/glew.h>
#ifdef _WIN32
#include <GL/wglew.h>
#endif
#endif

#endif // G2D_GL_H_
