//
// sdl2_extras.h
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 03/04/2016
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_EXTRAS_H
#define SPNLIB_SDL2_EXTRAS_H

#include <SDL2/SDL.h>

#include <spn/ctx.h>
#include <spn/private.h>

int spnlib_SDL_GetPaths(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_GetVersion(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_GetPlatform(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_GetCPUSpecs(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_GetPowerInfo(SpnValue *ret, int argc, SpnValue *argv, void *ctx);

// Set of SDL specifc macros
#define SPN_SDLBOOL(val) makebool(val == SDL_TRUE)

#endif
