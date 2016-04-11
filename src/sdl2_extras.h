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
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <spn/ctx.h>
#include <spn/private.h>

int spnlib_SDL_GetPaths(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_GetVersions(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_GetPlatform(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_GetCPUSpecs(SpnValue *ret, int argc, SpnValue *argv, void *ctx);
int spnlib_SDL_GetPowerInfo(SpnValue *ret, int argc, SpnValue *argv, void *ctx);

// Set of SDL specifc macros
#define SPN_SDLBOOL(val) (spn_makebool((val) != SDL_FALSE))

#endif
