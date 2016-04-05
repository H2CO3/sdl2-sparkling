//
// sdl2_sparkling.h
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 30/03/2015
//
// Licensed under the 2-clause BSD License
//

#ifndef SPNLIB_SDL2_H
#define SPNLIB_SDL2_H

#include <assert.h>
#include <stdbool.h>

#include <spn/ctx.h>
#include <spn/str.h>

static inline void spnlib_sdl2_argindex_oob(int index, int argc, SpnContext *ctx)
{
	const void *args[] = { &index, &argc };
	spn_ctx_runtime_error(
		ctx,
		"argument #%i is out of bounds (%i given)",
		args
	);
}

static inline void spnlib_sdl2_argtype_mismatch(
	int index,
	const char *desired,
	SpnValue *argv,
	SpnContext *ctx
)
{
	const void *args[] = { &index, desired, spn_type_name(argv[index].type) };
	spn_ctx_runtime_error(
		ctx,
		"argument #%i must be %s (was %s)",
		args
	);
}

// Prototyping helper
SpnValue spn_get_lib_prototype(const char *classname);

// Macros for checking for certain types of arguments
#define CHECK_ARG_RETURN_ON_ERROR(index, t)								\
	do {																\
		int index_s = index;											\
		if (index_s >= argc) {											\
			spnlib_sdl2_argindex_oob(index_s, argc, ctx);				\
			return -1;													\
		}																\
																		\
		if (!spn_is##t(&argv[index])) {									\
			spnlib_sdl2_argtype_mismatch(index_s, #t, argv, ctx);		\
			return -1;													\
		}																\
	} while (0)

#define BOOLARG(index) spn_boolvalue(&argv[index])
#define INTARG(index) spn_intvalue(&argv[index])
#define FLOATARG(index) spn_floatvalue(&argv[index])
#define NUMARG(index) spn_floatvalue_f(&argv[index])
#define STRARG(index) (spn_stringvalue(&argv[index])->cstr)
#define STRLENARG(index) (spn_stringvalue(&argv[index])->len)
#define ARRAYARG(index) spn_arrayvalue(&argv[index])
#define HASHMAPARG(index) spn_hashmapvalue(&argv[index])
#define FUNCARG(index) spn_funcvalue(&argv[index])
#define OBJARG(index) spn_objvalue(&argv[index])


// Classes used for binding SDL types to Sparkling
enum {
	SPN_SDL_CLASS_UID_BASE    = SPN_USER_CLASS_UID_BASE + (('S' << 16) | ('D' << 8) | ('L' << 0)),
	SPN_SDL_CLASS_UID_WINDOW  = SPN_SDL_CLASS_UID_BASE + 1,
	SPN_SDL_CLASS_UID_TIMER   = SPN_SDL_CLASS_UID_BASE + 2,
	SPN_SDL_CLASS_UID_TEXTURE = SPN_SDL_CLASS_UID_BASE + 3,
	SPN_SDL_CLASS_UID_AUDIO   = SPN_SDL_CLASS_UID_BASE + 4
};

#endif // SPNLIB_SDL2_H
