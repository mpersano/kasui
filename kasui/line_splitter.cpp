#include <cassert>
#include <cstring>

#include "guava2d/xwchar.h"
#include "guava2d/font.h"

#include "line_splitter.h"

line_splitter::line_splitter(const g2d::font *font, const wchar_t *str)
: font_(font)
, str_(str)
{ }

wchar_t *
line_splitter::next_line(int max_size)
{
	if (!*str_)
		return 0;

	const wchar_t *line_end = 0;

	int size = 0;
	const wchar_t *p = str_;
	wchar_t ch;

	do {
		ch = *p++;
		if (ch == L'\0' || ch == ' ')
			line_end = p - 1;
	} while (ch != L'\0' && (size += font_->find_glyph(ch)->advance_x) < max_size);

	assert(line_end);

	size_t result_len = line_end - str_;

	wchar_t *result = new wchar_t[result_len + 1];
	memcpy(result, str_, result_len*sizeof *result);
	result[result_len] = L'\0';

	str_ = *line_end ? line_end + 1 : line_end;

	return result;
}
