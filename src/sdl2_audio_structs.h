//
// sdl2_audio_structs.h
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 22/04/2016
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_AUDIO_STRUCTS_H
#define SPNLIB_SDL2_AUDIO_STRUCTS_H

#include "sdl2_sparkling.h"

//
// Music
//
typedef struct spn_SDL_Music {
	SpnObject base;
	Mix_Music *music;
} spn_SDL_Music;

spn_SDL_Music *from_hashmap_grab_Music(SpnHashMap *hm);

//
// Sample
//
typedef struct spn_SDL_Sample {
	SpnObject base;
	Mix_Chunk *chunk;
} spn_SDL_Sample;

spn_SDL_Sample *from_hashmap_grab_Sample(SpnHashMap *hm);

//
// Channels
//
typedef struct spn_SDL_Channels {
	SpnObject base;
} spn_SDL_Channels;

//
// Various
//
typedef struct {
	SpnContext *ctx;
	SpnFunction *fn;
	// Return values and arguments must be handled inside the callback function
} CallbackData;


#endif // SPNLIB_SDL2_AUDIO_STRUCTS_H
