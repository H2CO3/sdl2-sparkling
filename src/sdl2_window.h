//
// sdl2_window.h
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 02/04/2016
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_WINDOW_H
#define SPNLIB_SDL2_WINDOW_H

#include <spn/ctx.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>


int spnlib_SDL_OpenWindow(SpnValue *ret, int argc, SpnValue *argv, void *ctx);

// In order to bridge data between sdl2_sparkling.c and this file
void spnlib_SDL_methods_for_Window(SpnHashMap *window);

#endif
