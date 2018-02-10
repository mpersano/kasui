#include <cstring>
#include <cstdlib>

#include "guava2d/file.h"

#include "panic.h"
#include "utf8.h"
#include "kanji_info.h"

static const char *KANJI_INFO_FILE_PATH = "data/kanji";

std::vector<kanji_info *> kanji_info_list;

void
kanji_info_initialize()
{
	g2d::file_input_stream reader(KANJI_INFO_FILE_PATH);

	static char line[1024];

	while (reader.gets(line, sizeof line)) {
		char *code_utf8 = strtok(line, "\t");
		char *on_utf8 = strtok(nullptr, "\t");
		char *kun_utf8 = strtok(nullptr, "\t");
		char *meaning_utf8 = strtok(nullptr, "\t");
		char *level = strtok(nullptr, "\t\r\n");

		if (!code_utf8 || !on_utf8 || !kun_utf8 || !meaning_utf8 || !level)
			panic("invalid kanji info file");

		wchar_t *code = utf8_to_wchar(code_utf8);

		kanji_info_list.push_back(
			new kanji_info(
				code[0],
				utf8_to_wchar(on_utf8),
				utf8_to_wchar(kun_utf8),
				utf8_to_wchar(meaning_utf8),
				atoi(level)));

		delete[] code;
	}
}
