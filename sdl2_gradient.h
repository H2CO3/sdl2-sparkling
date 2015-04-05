//
// sdl2_gradient.h
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 05/04/2015
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_GRADIENT_H
#define SPNLIB_SDL2_GRADIENT_H

#include <spn/api.h>
#include <spn/array.h>
#include <SDL2/SDL.h>

typedef struct SPN_SDL_ColorStop {
	SDL_Color color;
	double p;
} SPN_SDL_ColorStop;

SPN_API bool spnlib_sdl2_array_to_colorstop(
	SpnArray *arr,
	SPN_SDL_ColorStop color_stops[]
);

SPN_API void spnlib_sdl2_linear_gradient(
	SDL_Renderer *renderer,
	SDL_Point start,
	SDL_Point end,
	double m,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n
);

SPN_API void spnlib_sdl2_radial_gradient(
	SDL_Renderer *renderer,
	SDL_Point center,
	int rx,
	int ry,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n
);

SPN_API void spnlib_sdl2_conical_gradient(
	SDL_Renderer *renderer,
	SDL_Point center,
	int rx,
	int ry,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n
);

#endif // SPNLIB_SDL2_GRADIENT_H
