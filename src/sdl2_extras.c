//
// sdl2_extras.c
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 03/04/2016
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_extras.h"
#include "sdl2_sparkling.h"


/////////////////////////////////
/////     Error Systems     /////
/////////////////////////////////

int set_sdl_error(
	int argc,
	SpnValue *argv,
	SpnContext *ctx,
	int (*Setter)(const char *fmt, ...)
)
{
	char *errmsg;
	SpnString *fmt = spn_stringvalue(&argv[0]);
	SpnString *res = spn_string_format_obj(fmt, argc - 1, &argv[1], &errmsg);

	if (res != NULL) {
		// next line seems convoluted but it shuts a warning
		Setter("%s", res->cstr);
		spn_object_release(res);
	} else {
		const void *args[1];
		args[0] = errmsg;
		spn_ctx_runtime_error(ctx, "error in format string: %s", args);
		free(errmsg);
		return -3;
	}

	return 0;
}

int spnlib_SDL_GetError(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makestring_nocopy(SDL_GetError());
	return 0;
}

int spnlib_SDL_SetError(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, string);
	return set_sdl_error(argc, argv, ctx, SDL_SetError);
}

int spnlib_SDL_GetMixError(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makestring_nocopy(Mix_GetError());
	return 0;
}

int spnlib_SDL_SetMixError(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, string);
	return set_sdl_error(argc, argv, ctx, Mix_SetError);
}

/////////////////////////////////
//////   Just some paths   //////
/////////////////////////////////
static char *get_pref_path(const char *org, const char *app)
{
	if (org != NULL && app != NULL) {
		return SDL_GetPrefPath(org, app);
	}

	return NULL;
}

static void set_and_free_sdl_path_string(SpnHashMap *paths, const char *key, char *path_ptr)
{
	SpnValue path = path_ptr ? spn_makestring(path_ptr) : spn_nilval;
	spn_hashmap_set_strkey(paths, key, &path);
	spn_value_release(&path);
	SDL_free(path_ptr);
}

int spnlib_SDL_GetPaths(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
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

	set_and_free_sdl_path_string(paths, "base", SDL_GetBasePath());
	set_and_free_sdl_path_string(paths, "pref", get_pref_path(org, app));

	return 0;
}

/////////////////////////////////
//////    SDL's version    //////
/////////////////////////////////
static void fill_sub_hashmap_version(SpnValue *lib, const SDL_version *version)
{
	SpnHashMap *submap = spn_hashmapvalue(lib);

	SpnValue major = spn_makeint(version->major);
	SpnValue minor = spn_makeint(version->minor);
	SpnValue patch = spn_makeint(version->patch);

	spn_hashmap_set_strkey(submap, "major", &major);
	spn_hashmap_set_strkey(submap, "minor", &minor);
	spn_hashmap_set_strkey(submap, "patch", &patch);

	spn_value_release(&major);
	spn_value_release(&minor);
	spn_value_release(&patch);
}

static void set_sub_hashmap(
	SpnHashMap *hm,
	const char *key,
	const SDL_version *version
)
{
	SpnValue lib = spn_makehashmap();

	fill_sub_hashmap_version(&lib, version);
	spn_hashmap_set_strkey(hm, key, &lib);

	spn_value_release(&lib);
}

int spnlib_SDL_GetVersions(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makehashmap();
	SpnHashMap *versions = spn_hashmapvalue(ret);

	// Version values
	SDL_version v_sdl; SDL_GetVersion(&v_sdl);
	SDL_version v_sdl_gfx = {
		SDL2_GFXPRIMITIVES_MAJOR,
		SDL2_GFXPRIMITIVES_MINOR,
		SDL2_GFXPRIMITIVES_MICRO
	};

	// Setting sub-hashmaps
	set_sub_hashmap(versions, "sdl", &v_sdl);
	set_sub_hashmap(versions, "sdl_gfx", &v_sdl_gfx);
	set_sub_hashmap(versions, "sdl_image", IMG_Linked_Version());
	set_sub_hashmap(versions, "sdl_ttf", TTF_Linked_Version());
	set_sub_hashmap(versions, "sdl_mixer", Mix_Linked_Version());

	return 0;
}

/////////////////////////////////
////    Platform and CPU    /////
/////////////////////////////////
int spnlib_SDL_GetPlatform(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{	// Simplest function to bind
	*ret = spn_makestring_nocopy(SDL_GetPlatform());
	return 0;
}

int spnlib_SDL_GetCPUSpecs(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makehashmap();
	SpnHashMap *cpu_features = spn_hashmapvalue(ret);

	// These are integers
	SpnValue cache = spn_makeint(SDL_GetCPUCacheLineSize());
	SpnValue cores = spn_makeint(SDL_GetCPUCount());
	SpnValue ram   = spn_makeint(SDL_GetSystemRAM());

	// And these are booleans
	static const struct {
		const char *name;
		SDL_bool (*fn)(void);
	} bool_specs[] = {
		{ "3dnow",   SDL_Has3DNow   },
		{ "avx",     SDL_HasAVX     },
		{ "avx2",    SDL_HasAVX2    },
		{ "altivec", SDL_HasAltiVec },
		{ "mmx",     SDL_HasMMX     },
		{ "rdtsc",   SDL_HasRDTSC   },
		{ "sse",     SDL_HasSSE     },
		{ "sse2",    SDL_HasSSE2    },
		{ "sse3",    SDL_HasSSE3    },
		{ "sse41",   SDL_HasSSE41   },
		{ "sse42",   SDL_HasSSE42   },
	};

	// Set integer properties
	spn_hashmap_set_strkey(cpu_features, "cache", &cache);
	spn_hashmap_set_strkey(cpu_features, "cores", &cores);
	spn_hashmap_set_strkey(cpu_features, "ram",   &ram);

	// Set Boolean properties
	for (size_t i = 0; i < COUNT(bool_specs); i++) {
		SpnValue val = SPN_SDLBOOL(bool_specs[i].fn());
		spn_hashmap_set_strkey(cpu_features, bool_specs[i].name, &val);
	}

	return 0;
}

/////////////////////////////////
////    Power Management    /////
/////////////////////////////////
static const char *powerstate_to_string(SDL_PowerState state)
{
	switch (state) {
	case SDL_POWERSTATE_ON_BATTERY: return "not plugged in, running on battery";
	case SDL_POWERSTATE_NO_BATTERY: return "plugged in, no battery available";
	case SDL_POWERSTATE_CHARGING:   return "plugged in, charging battery";
	case SDL_POWERSTATE_CHARGED:    return "plugged in, battery charged";
	case SDL_POWERSTATE_UNKNOWN: // FALLTHRU
	default:                        return "cannot determine power status";
	}
}

int spnlib_SDL_GetPowerInfo(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makehashmap();
	SpnHashMap *powerinfo = spn_hashmapvalue(ret);

	int secs, pct;
	SDL_PowerState state = SDL_GetPowerInfo(&secs, &pct);

	SpnValue str = spn_makestring_nocopy(powerstate_to_string(state));
	SpnValue seconds = spn_makeint(secs);
	SpnValue percentage = spn_makeint(pct);

	spn_hashmap_set_strkey(powerinfo, "state", &str);
	spn_hashmap_set_strkey(powerinfo, "seconds", &seconds);
	spn_hashmap_set_strkey(powerinfo, "percentage", &percentage);

	spn_value_release(&str);
	spn_value_release(&seconds);
	spn_value_release(&percentage);

	return 0;
}

/////////////////////////////////
////         Delay          /////
/////////////////////////////////
int spnlib_SDL_Delay(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, int);
	SDL_Delay(INTARG(0));
	return 0;
}
