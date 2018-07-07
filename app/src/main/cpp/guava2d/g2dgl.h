#pragma once

#include "panic.h"

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

#ifdef CHECK_GL_CALLS
#define GL_CHECK(expr) \
    [&] { \
        expr; \
        auto e = glGetError(); \
        if (e != GL_NO_ERROR) \
        panic("%s:%d: GL error: %x", __FILE__, __LINE__, e); \
    }()

#define GL_CHECK_R(expr) \
    [&] { \
        auto r = expr; \
        auto e = glGetError(); \
        if (e != GL_NO_ERROR) \
            panic("%s:%d: GL error: %x", __FILE__, __LINE__, e); \
        return r; \
    }()
#else
#define GL_CHECK(expr) expr
#define GL_CHECK_R(expr) expr
#endif
