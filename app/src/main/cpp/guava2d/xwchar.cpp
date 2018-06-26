#include <cstdio>
#include <stdarg.h>

#include "xwchar.h"

static wchar_t *
format_string(wchar_t *dest, const wchar_t *str, bool zero_pad, int width)
{
	if (width > 0) {
		for (const wchar_t *p = str; *p; p++)
			--width;

		while (width-- > 0)
			*dest++ = zero_pad ? L'0' : L' ';
	}

	while (*str)
		*dest++ = *str++;

	return dest;
}

static wchar_t *
format_unsigned(wchar_t *dest, unsigned value, bool zero_pad, int width)
{
	wchar_t buf[20] = {0};

	wchar_t *p;

	if (value) {
		p = &buf[sizeof buf/sizeof *buf - 1];

		while (value) {
			*--p = value%10 + L'0';
			value /= 10;
		}
	} else {
		p = const_cast<wchar_t *>(L"0"); // MUHAHAH
	}

	return format_string(dest, p, zero_pad, width);
}

void
xvswprintf(wchar_t *str, const wchar_t *fmt, va_list ap) 
{
	enum state {
		STATE_TEXT,
		STATE_AFTER_PERCENT,
		STATE_WIDTH,
	};

	state cur_state = STATE_TEXT;

	bool zero_pad = false;
	int width = 0;

	wchar_t *q = str;

	for (const wchar_t *p = fmt; *p; p++) {
		const wchar_t ch = *p;

		switch (cur_state) {
			case STATE_TEXT:
				if (ch == L'%') {
					zero_pad = false;
					width = -1;
					cur_state = STATE_AFTER_PERCENT;
				} else {
					*q++ = ch;
				}
				break;

			case STATE_AFTER_PERCENT:
				if (ch == L'%') {
					*q++ = L'%';
					cur_state = STATE_TEXT;
				} else if (ch == L'0') {
					zero_pad = true;
				} else if (ch > L'0' && ch <= L'9') {
					width = ch - L'0';
					cur_state = STATE_WIDTH;
				} else {
					goto format_arg;
				}
				break;

			case STATE_WIDTH:
				if (ch >= L'0' && ch <= L'9') {
					width = width*10 + ch - L'0';
				} else {
					format_arg:

					if (ch == L's')
						q = format_string(q, va_arg(ap, const wchar_t *), zero_pad, width);
					else if (ch == L'u' || ch == L'd')
						q = format_unsigned(q, va_arg(ap, unsigned), zero_pad, width);
					else if (ch == L'c')
						*q++ = va_arg(ap, int);

					cur_state = STATE_TEXT;
				}
				break;
		}
	}

	va_end(ap);

	*q = '\0';
}

void
xswprintf(wchar_t *str, const wchar_t *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	xvswprintf(str, fmt, ap);
	va_end(ap);
}

size_t
xwcslen(const wchar_t *str)
{
	size_t len = 0;
	while (*str++)
		++len;
	return len;
}

void
xwcscpy(wchar_t *dest, const wchar_t *src)
{
	while ((*dest++ = *src++))
		;
}

wchar_t *
xwcsdup(const wchar_t *str)
{
	wchar_t *p = new wchar_t[xwcslen(str) + 1];
	xwcscpy(p, str);
	return p;
}
