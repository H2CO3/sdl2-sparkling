//
// sdl2_sparkling.c
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 28/03/2015
//
// Licensed under the 2-clause BSD License
//

#define USE_DYNAMIC_LOADING 1

#include <SDL2/SDL.h>

#include "sdl2_sparkling.h"
#include "sdl2_window.h"
#include "sdl2_event.h"
#include "sdl2_timer.h"


/////////////////////////////////
// The returned library object //
/////////////////////////////////
static SpnHashMap *library = NULL;

/////////////////////////////////
////////  Window class  /////////
/////////////////////////////////
// This is required to let "Window" continue to act as a namespace
SpnValue spn_get_window_prototype(void) {
	return spn_hashmap_get_strkey(library, "Window");
}

/////////////////////////////////
//////   Just some paths   //////
/////////////////////////////////
static const char *get_base_path(void)
{
	const char *base = SDL_GetBasePath();
	return base ? base : "";
}

static const char *get_pref_path(const char *org, const char *app)
{
	const char *pref = NULL;

	if (org != NULL && app != NULL) {
		pref = SDL_GetPrefPath(org, app);
	}

	return pref ? pref : "";
}

static int spnlib_SDL_GetPaths(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	if (argc >= 2) {
		CHECK_ARG_RETURN_ON_ERROR(0, string);
		CHECK_ARG_RETURN_ON_ERROR(1, string);
	}

	// construct return value: hashmap with strings
	*ret = spn_makehashmap();
	SpnHashMap *paths = spn_hashmapvalue(ret);

	// get proper arguments for Pref, if given
	const char *org = NULL, *app = NULL;
	if (argc >= 2) {
		org = STRARG(0);
		app = STRARG(1);
	}

	SpnValue base = spn_makestring_nocopy(get_base_path());
	SpnValue pref = spn_makestring_nocopy(get_pref_path(org, app));

	spn_hashmap_set_strkey(paths, "base", &base);
	spn_hashmap_set_strkey(paths, "pref", &pref);

	spn_value_release(&base);
	spn_value_release(&pref);

	return 0;
}

//
// Library initialization and deinitialization
//

// the library has been loaded this many times
static unsigned init_refcount = 0;

// helper for building the hashmap representing the library
static void spn_SDL_construct_library(void)
{
	assert(library == NULL);
	library = spn_hashmap_new();

	// top-level library functions
	static const SpnExtFunc fns[] = {
		{ "OpenWindow",  spnlib_SDL_OpenWindow   },
		{ "PollEvent",   spnlib_SDL_PollEvent    },
		{ "StartTimer",  spnlib_SDL_StartTimer   },
		{ "StopTimer",   spnlib_SDL_StopTimer    },
		{ "GetPaths",    spnlib_SDL_GetPaths     }
	};

	for (size_t i = 0; i < sizeof fns / sizeof fns[0]; i++) {
		SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
		spn_hashmap_set_strkey(library, fns[i].name, &fnval);
		spn_value_release(&fnval);
	}

	SpnHashMap *window = spn_hashmap_new();
	spnlib_SDL_Window_methods(window);
	spn_hashmap_set_strkey(
		library,
		"Window",
		&(SpnValue){ .type = SPN_TYPE_HASHMAP, .v.o = window }
	);
	spn_object_release(window);
}

// when the last reference is gone to our library, we free the resources
static void spn_SDL_destroy_library(void)
{
	assert(library);
	spn_object_release(library);
	library = NULL;
}


// Library constructor and destructor
SPN_LIB_OPEN_FUNC(ctx) {
	if (init_refcount == 0) {
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			fprintf(stderr, "can't initialize SDL: %s\n", SDL_GetError());
			return spn_nilval;
		}

		spn_SDL_construct_library();
	}

	init_refcount++;

	spn_object_retain(library);
	return (SpnValue){ .type = SPN_TYPE_HASHMAP, .v.o = library };
}

SPN_LIB_CLOSE_FUNC(ctx) {
	if (--init_refcount == 0) {
		// free hashmap representing library
		spn_SDL_destroy_library();

		// deinitialize SDL
		SDL_Quit();
	}
}
