#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <wchar.h>

#include "guava2d/file.h"

#include "panic.h"
#include "utf8.h"
#include "jukugo.h"

static const char *JUKUGO_FILE_PATH = "data/jukugo";

unsigned jukugo_list_size;
jukugo **jukugo_list;

void
jukugo_initialize()
{
	g2d::file_input_stream reader(JUKUGO_FILE_PATH);

	jukugo_list_size = 0;

	unsigned jukugo_list_capacity;
	jukugo_list = new jukugo *[jukugo_list_capacity = 32];

	static char line[1024];

	while (reader.gets(line, sizeof line)) {
		char *kanji_utf8 = strtok(line, "\t");
		char *reading_utf8 = strtok(0, "\t");
		char *eigo_utf8 = strtok(0, "\t");
		char *level = strtok(0, "\t\r\n");

		if (!kanji_utf8 || !reading_utf8 || !eigo_utf8 || !level)
			panic("invalid jukugo file");

		if (jukugo_list_size == jukugo_list_capacity) {
			jukugo **new_jukugo_list = new jukugo *[jukugo_list_capacity *= 2];

			// yes, i'd love to use std::copy (or std::vector for that matter) but
			// this is android. no stl.
			memcpy(new_jukugo_list, jukugo_list, jukugo_list_size*sizeof *jukugo_list);
			delete[] jukugo_list;

			jukugo_list = new_jukugo_list;
		}

		jukugo_list[jukugo_list_size++] =
			new jukugo(
				utf8_to_wchar(kanji_utf8),
				utf8_to_wchar(reading_utf8),
				utf8_to_wchar(eigo_utf8),
				atoi(level));
	}
}

void
jukugo_load_hits(const char *path)
{
	FILE *in;

	if ((in = fopen(path, "r")) != 0) {
		for (int i = 0; i < jukugo_list_size; i++) {
			jukugo *p = jukugo_list[i];
			fread(&p->hits, sizeof(p->hits), 1, in);
		}

		fclose(in);
	}
}

void
jukugo_save_hits(const char *path)
{
	FILE *out;

	if ((out = fopen(path, "w")) != 0) {
		for (int i = 0; i < jukugo_list_size; i++) {
			const jukugo *p = jukugo_list[i];
			fwrite(&p->hits, sizeof(p->hits), 1, out);
		}

		fclose(out);
	}
}
