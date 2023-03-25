#ifndef __T_MUSIC_H__
#define __T_MUSIC_H__

#include "notes.h"

typedef struct {
	uint16_t frequency;
	int8_t duration;
} note_t;

typedef struct { // IBM
	uint16_t tempo;
	note_t notes[];
} sound_data_t;

#endif /* ! __T_MUSIC_H__ */
