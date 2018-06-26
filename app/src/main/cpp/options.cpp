#include <cstdio>
#include <cstring>

#include <cassert>

#include <ctype.h>
#include <stdlib.h>

#include <guava2d/xwchar.h>

#include "options.h"
#include "utf8.h"

namespace {

enum option_type
{
    OPTION_NONE,
    OPTION_INTEGER,
    OPTION_STRING
};

const struct name_to_option
{
    const char *name;
    option_type type;
    int options::*value;
} name_to_options[] = {
    {"enable-hints", OPTION_INTEGER, &options::enable_hints},
    {"max-unlocked-level", OPTION_INTEGER, &options::max_unlocked_level},
    {"enable-sound", OPTION_INTEGER, &options::enable_sound},
    {"player-name", OPTION_STRING, reinterpret_cast<int options::*>(&options::player_name)},
    {nullptr, OPTION_NONE, nullptr},
};

} // namespace

options::options()
    : enable_hints(true)
    , max_unlocked_level(0)
    , enable_sound(false)
    , player_name(nullptr)
{
}

options *options::load(const char *path)
{
    options *o = nullptr;

    FILE *in;

    if ((in = fopen(path, "r")) != nullptr) {
        o = new options;

        static char line[256];

        while (fgets(line, sizeof line, in)) {
            char *name = strtok(line, " ");
            char *value = strtok(nullptr, " ");
            if (!value)
                break;

            size_t len = strlen(value);
            while (len > 0 && isspace(value[len - 1]))
                value[--len] = 0;

            for (const name_to_option *p = name_to_options; p->name; p++) {
                if (!strcmp(p->name, name)) {
                    switch (p->type) {
                        case OPTION_INTEGER:
                            o->*p->value = atoi(value);
                            break;

                        case OPTION_STRING:
                            o->*reinterpret_cast<wchar_t * options::*>(p->value) = utf8_to_wchar(value);
                            break;

                        default:
                            assert(0);
                    }
                    break;
                }
            }
        }

        fclose(in);
    }

    return o;
}

void options::save(const char *path) const
{
    FILE *out;

    if ((out = fopen(path, "w")) != nullptr) {
        for (const name_to_option *p = name_to_options; p->name; p++) {
            switch (p->type) {
                case OPTION_INTEGER:
                    fprintf(out, "%s %d\n", p->name, this->*p->value);
                    break;

                case OPTION_STRING:
                    if (const wchar_t *value = this->*reinterpret_cast<wchar_t * options::*>(p->value)) {
                        unsigned char *value_utf8 = wchar_to_utf8(value);
                        fprintf(out, "%s %s\n", p->name, value_utf8);
                        delete[] value_utf8;
                    }
                    break;

                default:
                    assert(0);
            }
        }

        fclose(out);
    }
}

void options::set_player_name(const wchar_t *name)
{
    if (player_name)
        delete[] player_name;

    player_name = xwcsdup(name);
}
