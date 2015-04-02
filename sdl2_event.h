//
// sdl2_event.h
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 02/04/2015
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_EVENT_H
#define SPNLIB_SDL2_EVENT_H

#include <spn/ctx.h>
#include <spn/api.h>

SPN_API int spnlib_SDL_PollEvent(SpnValue *ret, int argc, SpnValue *argv, void *ctx);

#endif // SPNLIB_SDL2_EVENT_H
