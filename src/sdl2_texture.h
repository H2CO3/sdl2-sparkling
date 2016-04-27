//
// texture.h
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 02/04/2015
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL_TEXTURE_H
#define SPNLIB_SDL_TEXTURE_H

#include <SDL2/SDL.h>
#include <spn/api.h>

typedef struct spn_SDL_Texture {
	SpnObject base;
	SDL_Texture *texture;
} spn_SDL_Texture;

extern const SpnClass spn_SDL_Texture_class;

// Transfers ownership of 'texture'
SPN_API spn_SDL_Texture *spnlib_SDL_texture_new(SDL_Texture *texture);

// Deallocates 'surface'
SPN_API spn_SDL_Texture *spnlib_SDL_texture_new_surface(
	SDL_Renderer *renderer,
	SDL_Surface *surface
);

#endif // SPNLIB_SDL_TEXTURE_H
