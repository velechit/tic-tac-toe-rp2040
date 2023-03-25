#ifndef __POP2040_MUSIC_H
#define __POP2040_MUSIC_H

#include "tmusic.h"



void music_init(void);
void play_music(const sound_data_t *music_data);
void stop_sound(void);
bool is_music_playing(void);

#endif /* ! __POP2040_MUSIC_H */