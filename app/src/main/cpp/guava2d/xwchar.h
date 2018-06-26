#ifndef XWCHAR_H_
#define XWCHAR_H_

#include <stdarg.h>
#include <wchar.h>

// WCHAR IS BROKEN ON ANDROID#@!!!

void xswprintf(wchar_t *str, const wchar_t *fmt, ...);
void xvswprintf(wchar_t *str, const wchar_t *fmt, va_list ap);

size_t xwcslen(const wchar_t *str);
void xwcscpy(wchar_t *dest, const wchar_t *src);

wchar_t *xwcsdup(const wchar_t *str);

#endif // XWCHAR_H_
