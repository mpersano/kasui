#pragma once

#include <vector>

struct jukugo
{
    wchar_t *kanji;
    wchar_t *reading;
    wchar_t *eigo;
    int level;
    int hits;
};

extern std::vector<jukugo> jukugo_list;

void jukugo_initialize();

void jukugo_load_hits(const char *path);

void jukugo_save_hits(const char *path);
