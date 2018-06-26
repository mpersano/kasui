#pragma once

#include "guava2d/g2dgl.h"
#include "panic.h"

#if 1

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
