#include <climits>

#include "options.h"
#include "sounds.h"

#ifdef ENABLE_SOUND
#include <SDL_mixer.h>
static Mix_Chunk *sounds[NUM_SOUNDS];
#endif

void sounds_initialize()
{
#ifdef ENABLE_SOUND
    const char *sound_sources[NUM_SOUNDS] = {
        "opening.wav",   "menu_select.wav",     "menu_validate.wav", "menu_back.wav",   "level_intro.wav",
        "game_over.wav", "level_completed.wav", "countdown.wav",     "block_match.wav", "block_miss.wav",
    };

    for (int i = 0; i < NUM_SOUNDS; i++) {
        char path[PATH_MAX];
        snprintf(path, sizeof path, "sounds/%s", sound_sources[i]);

        if ((sounds[i] = Mix_LoadWAV(path)) == NULL)
            fprintf(stderr, "failed to open %s: %s\n", path, Mix_GetError());
    }

    Mix_AllocateChannels(NUM_SOUNDS); // one channel per sound
#endif
}

void sounds_release()
{
#ifdef ENABLE_SOUND
    for (int i = 0; i < NUM_SOUNDS; i++)
        Mix_FreeChunk(sounds[i]);
#endif
}

void start_sound(int id, bool loop)
{
#ifdef ENABLE_SOUND
    extern options *cur_options;

    if (cur_options->enable_sound)
        Mix_PlayChannel(id, sounds[id], loop ? -1 : 0);
#endif
}

void stop_sound(int id)
{
#ifdef ENABLE_SOUND
    extern options *cur_options;

    if (cur_options->enable_sound)
        Mix_HaltChannel(id);
#endif
}

void stop_all_sounds()
{
#ifdef ENABLE_SOUND
    for (int i = 0; i < NUM_SOUNDS; i++)
        stop_sound(i);
#endif
}
