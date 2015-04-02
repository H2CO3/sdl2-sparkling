//
// sdl2_ttf.h
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 30/03/2015
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_TTF_H
#define SPNLIB_SDL2_TTF_H

#include <spn/api.h>

#include <SDL2/SDL.h>
#include <SDL2/sdl_ttf.h>

SPN_API TTF_Font *spnlib_sdl2_get_font(
	const char *name,
	int ptsize,
	const char *style
);

SPN_API void spnlib_sdl2_render_text(
	SDL_Renderer *renderer,
	int x,
	int y,
	const char *text,
	TTF_Font *font,
	bool hq // false: fast, true: high-quality
);

#endif // SPNLIB_SDL2_TTF_H
