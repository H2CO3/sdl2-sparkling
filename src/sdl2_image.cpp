//
// sdl2_image.cpp
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 02/04/2015
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_image.h"

#include <SDL2/SDL_image.h>

static const struct IMG_InitGuard {
	IMG_InitGuard() {
		IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
	}
	~IMG_InitGuard() {
		IMG_Quit();
	}
} initGuard;

spn_SDL_Texture *spnlib_sdl2_load_image(
	SDL_Renderer *renderer,
	const char *filename
)
{
	SDL_Surface *surface = IMG_Load(filename);

	if (surface == nullptr) {
		return nullptr;
	}

	return spnlib_SDL_texture_new_surface(renderer, surface);
}
