#ifndef LINE_SPLITTER_H_
#define LINE_SPLITTER_H_

#include <wchar.h>

namespace g2d {
class font;
};

class line_splitter
{
public:
	line_splitter(const g2d::font *font, const wchar_t *str);

	std::wstring next_line(int size);

private:
	const g2d::font *font_;
	const wchar_t *str_;
};

#endif // LINE_SPLITTER_H_
