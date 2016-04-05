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
#include <SDL2/SDL.h>


int spnlib_SDL_OpenAudioDevice(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_ListAudioDevices(SpnValue *ret, int argc, SpnValue *argv, void *ctx);

// In order to bridge data between sdl2_sparkling.c and this file
void spnlib_SDL_methods_for_Audio(SpnHashMap *audio);

#endif // SPNLIB_SDL2_AUDIO_H
