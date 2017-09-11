#ifndef SOUNDS_H_
#define SOUNDS_H_

enum {
	SOUND_OPENING,
	SOUND_MENU_SELECT,
	SOUND_MENU_VALIDATE,
	SOUND_MENU_BACK,
	SOUND_LEVEL_INTRO,
	SOUND_GAME_OVER,
	SOUND_LEVEL_COMPLETED,
	SOUND_COUNTDOWN,
	SOUND_BLOCK_MATCH,
	SOUND_BLOCK_MISS,
	NUM_SOUNDS,
};

void start_sound(int id, bool loop);
void stop_sound(int id);
void stop_all_sounds();

#endif // SOUNDS_H_
