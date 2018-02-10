#ifndef JUKUGO_H_
#define JUKUGO_H_

struct jukugo
{
    jukugo(wchar_t *kanji, wchar_t *reading, wchar_t *eigo, int level)
        : kanji(kanji)
        , reading(reading)
        , eigo(eigo)
        , level(level)
        , hits(0)
    {
    }

    wchar_t *kanji;
    wchar_t *reading;
    wchar_t *eigo;
    int level;
    int hits;
};

extern jukugo **jukugo_list;
extern unsigned jukugo_list_size;

void jukugo_initialize();

void jukugo_load_hits(const char *path);

void jukugo_save_hits(const char *path);

#endif // JUKUGO_H_
