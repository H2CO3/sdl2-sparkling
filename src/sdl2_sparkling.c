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
#include "sdl2_extras.h"


/////////////////////////////////
// The returned library object //
/////////////////////////////////
static SpnHashMap *library = NULL;

/////////////////////////////////
////////  Window class  /////////
/////////////////////////////////
// This is required to let "Window" continue to act as a namespace
SpnValue spn_get_window_prototype(void)
{
	return spn_hashmap_get_strkey(library, "Window");
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
		{ "OpenWindow",   spnlib_SDL_OpenWindow   },
		{ "PollEvent",    spnlib_SDL_PollEvent    },
		{ "StartTimer",   spnlib_SDL_StartTimer   },
		{ "StopTimer",    spnlib_SDL_StopTimer    },
		{ "GetPaths",     spnlib_SDL_GetPaths     },
		{ "GetVersion",   spnlib_SDL_GetVersion   },
		{ "GetPlatform",  spnlib_SDL_GetPlatform  },
		{ "GetCPUSpecs",  spnlib_SDL_GetCPUSpecs  },
		{ "GetPowerInfo", spnlib_SDL_GetPowerInfo }
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
