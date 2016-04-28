//
// sdl2_audio.h
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 03/04/2016
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_AUDIO_H
#define SPNLIB_SDL2_AUDIO_H

#include <spn/api.h>
#include <spn/ctx.h>
#include <spn/private.h>

#include <SDL2/SDL_mixer.h>


// Library functions
int spnlib_SDL_OpenMusic(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_OpenSample(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_OpenChannels(SpnValue *ret, int argc, SpnValue *argv, void *ctx);

// In order to bridge data between files
void spnlib_SDL_methods_for_Music(SpnHashMap *audio);
void spnlib_SDL_methods_for_Sample(SpnHashMap *audio);
void spnlib_SDL_methods_for_Channels(SpnHashMap *audio);

// Ability to grab an audio object
#define CHECK_FOR_AUDIO_HASHMAP(argnum, type)                        \
	CHECK_ARG_RETURN_ON_ERROR(argnum, hashmap);                      \
	SpnHashMap *hm = HASHMAPARG(argnum);                             \
	spn_SDL_##type *type = from_hashmap_grab_##type(hm);             \
	if (type == NULL) {                                              \
		spn_ctx_runtime_error(ctx, "audio object is invalid", NULL); \
		return -1;                                                   \
	}

// Audio class functions
const char *get_audioformat_string(SDL_AudioFormat fmt);
SDL_AudioFormat get_audioformat_value(const char *name);
const char *get_channel_string(Uint8 channel);
const char *fade_to_string(Mix_Fading fade);
void fill_audio_hashmap_with_values(SpnHashMap *hm, int sample);
void set_decoder_list_array(SpnArray *arr, size_t count, const char *(*GetDecoder)(int));

#endif // SPNLIB_SDL2_AUDIO_H
