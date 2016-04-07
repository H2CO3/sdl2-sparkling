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
int spnlib_SDL_GetVersion(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makehashmap();
	SpnHashMap *version = spn_hashmapvalue(ret);

	SDL_version linked;
	SDL_GetVersion(&linked);

	SpnValue major = spn_makeint(linked.major);
	SpnValue minor = spn_makeint(linked.minor);
	SpnValue patch = spn_makeint(linked.patch);

	spn_hashmap_set_strkey(version, "major", &major);
	spn_hashmap_set_strkey(version, "minor", &minor);
	spn_hashmap_set_strkey(version, "patch", &patch);

	spn_value_release(&major);
	spn_value_release(&minor);
	spn_value_release(&patch);

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
	for (size_t i = 0; i < sizeof bool_specs / sizeof bool_specs[0]; i++) {
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
	case SDL_POWERSTATE_ON_BATTERY: return "not plugged in, running on the battery";
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
