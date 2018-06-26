#ifndef OPTIONS_H_
#define OPTIONS_H_

struct options
{
    options();

    static options *load(const char *path);
    void save(const char *path) const;

    void set_player_name(const wchar_t *name);

    int enable_hints;
    int max_unlocked_level;
    int enable_sound;
    const wchar_t *player_name;
};

#endif // OPTIONS_H_
