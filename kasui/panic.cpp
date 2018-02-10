#include <cstdio>
#include <stdarg.h>
#include <stdlib.h>

#include "panic.h"

void panic(const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "FATAL: ");

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);

    abort();
}
