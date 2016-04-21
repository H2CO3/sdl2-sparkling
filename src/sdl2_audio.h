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


int spnlib_SDL_OpenMusic(SpnValue *ret, int argc, SpnValue *argv, void *ctx);

// In order to bridge data between sparkling.c and this file
void spnlib_SDL_methods_for_Music(SpnHashMap *audio);

#endif // SPNLIB_SDL_AUDIO_H
