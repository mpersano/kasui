#ifndef KANJI_INFO_H_
#define KANJI_INFO_H_

#include <vector>

struct kanji_info
{
    kanji_info(wchar_t code, wchar_t *on, wchar_t *kun, wchar_t *meaning, int level)
        : code(code)
        , on(on)
        , kun(kun)
        , meaning(meaning)
        , level(level)
    {
    }

    wchar_t code;
    wchar_t *on;
    wchar_t *kun;
    wchar_t *meaning;
    int level;
};

extern std::vector<kanji_info *> kanji_info_list;

void kanji_info_initialize();

#endif // KANJI_INFO_H_
