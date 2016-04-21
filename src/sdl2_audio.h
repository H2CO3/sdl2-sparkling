//
// sdl2_audio.h
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 03/04/2016
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL_AUDIO_H
#define SPNLIB_SDL_AUDIO_H

#include <spn/api.h>
#include <spn/ctx.h>
#include <spn/private.h>

#include <SDL2/SDL_mixer.h>


// Library functions
int spnlib_SDL_OpenMusic(SpnValue *ret, int argc, SpnValue *argv, void *ctx);

// Audio class functions and lifesavers
#define CHECK_FOR_AUDIO_HASHMAP(type)                                 \
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);                            \
	SpnHashMap *hm = HASHMAPARG(0);                                   \
	spn_SDL_##type *mixer = audio_from_hashmap(hm);                   \
	if (mixer == NULL) {                                              \
		spn_ctx_runtime_error(ctx, "audio object is invalid", NULL);  \
		return -1;                                                    \
	}

const char *get_audioformat_string(SDL_AudioFormat fmt);
SDL_AudioFormat get_audioformat_value(const char *name);
const char *get_channel_string(Uint8 channel);
void fill_hashmap_with_values(SpnHashMap *hm, int sample);
void set_decoder_list_array(SpnArray *arr, size_t count, const char *(*GetDecoder)(int));

// In order to bridge data between sparkling.c and this file
void spnlib_SDL_methods_for_Music(SpnHashMap *audio);

#endif // SPNLIB_SDL_AUDIO_H
