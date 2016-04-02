//
// sdl2_texture.c
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 02/04/2015
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_texture.h"
#include "sdl2_sparkling.h"

static void spn_SDL_Texture_dtor(void *obj)
{
	spn_SDL_Texture *texture = obj;
	SDL_DestroyTexture(texture->texture);
}

// A simple RAII class for managing SDL_Texture objects
const SpnClass spn_SDL_Texture_class = {
	sizeof(spn_SDL_Texture),
	SPN_SDL_CLASS_UID_TEXTURE,
	NULL,
	NULL,
	NULL,
	spn_SDL_Texture_dtor
};

spn_SDL_Texture *spnlib_SDL_texture_new(SDL_Texture *texture)
{
	spn_SDL_Texture *obj = spn_object_new(&spn_SDL_Texture_class);
	obj->texture = texture;
	return obj;
}

spn_SDL_Texture *spnlib_SDL_texture_new_surface(
	SDL_Renderer *renderer,
	SDL_Surface *surface
)
{
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	return spnlib_SDL_texture_new(texture);
}
