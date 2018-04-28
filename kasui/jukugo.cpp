#include "jukugo.h"

#include "guava2d/file.h"

#include "panic.h"
#include "utf8.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <wchar.h>

static const char *JUKUGO_FILE_PATH = "data/jukugo";

std::vector<jukugo> jukugo_list;

void jukugo_initialize()
{
    g2d::file_input_stream reader(JUKUGO_FILE_PATH);

    static char line[1024];

    while (reader.gets(line, sizeof line)) {
        char *kanji_utf8 = strtok(line, "\t");
        char *reading_utf8 = strtok(nullptr, "\t");
        char *eigo_utf8 = strtok(nullptr, "\t");
        char *level = strtok(nullptr, "\t\r\n");

        if (!kanji_utf8 || !reading_utf8 || !eigo_utf8 || !level)
            panic("invalid jukugo file");

        jukugo_list.push_back({utf8_to_wchar(kanji_utf8), utf8_to_wchar(reading_utf8), utf8_to_wchar(eigo_utf8), atoi(level)});
    }
}

void jukugo_load_hits(const char *path)
{
    FILE *in;

    if ((in = fopen(path, "r")) != nullptr) {
        for (auto& jukugo : jukugo_list) {
            fread(&jukugo.hits, sizeof(jukugo.hits), 1, in);
        }

        fclose(in);
    }
}

void jukugo_save_hits(const char *path)
{
    FILE *out;

    if ((out = fopen(path, "w")) != nullptr) {
        for (const auto& jukugo : jukugo_list) {
            fwrite(&jukugo.hits, sizeof(jukugo.hits), 1, out);
        }

        fclose(out);
    }
}
