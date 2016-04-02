//
// sdl2_timer.h
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 02/04/2015
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_TIMER_H
#define SPNLIB_SDL2_TIMER_H

#include <spn/api.h>
#include <spn/ctx.h>
#include <SDL2/SDL.h>


int spnlib_SDL_StartTimer(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_StopTimer(SpnValue *ret, int argc, SpnValue *argv, void *ctx);

#endif // SPNLIB_SDL2_TIMER_H
