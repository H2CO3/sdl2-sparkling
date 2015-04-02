//
// sdl2_image.h
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 02/04/2015
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_IMAGE_H
#define SPNLIB_SDL2_IMAGE_H

#include <spn/api.h>

#include "sdl2_texture.h"

SPN_API spn_SDL_Texture *spnlib_sdl2_load_image(
	SDL_Renderer *renderer,
	const char *filename
);

#endif // SPNLIB_SDL2_IMAGE_H
