//
// helpers.h
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 12/04/2016
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL_HELPERS_H
#define SPNLIB_SDL_HELPERS_H

#include <spn/ctx.h>
#include <spn/api.h>
#include <spn/str.h>

double constrain_to_01(double x);

void set_integer_property(SpnHashMap *hm, const char *name, long n);
void set_float_property(SpnHashMap *hm, const char *name, double x);
void set_string_property(SpnHashMap *hm, const char *name, const char *str);
void set_string_property_nocopy(SpnHashMap *hm, const char *name, const char *str);

#endif // SPNLIB_SDL_HELPERS_H
