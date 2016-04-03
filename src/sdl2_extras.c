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

	SpnValue base = spn_makestring_nocopy(get_base_path());
	SpnValue pref = spn_makestring_nocopy(get_pref_path(org, app));

	spn_hashmap_set_strkey(paths, "base", &base);
	spn_hashmap_set_strkey(paths, "pref", &pref);

	spn_value_release(&base);
	spn_value_release(&pref);

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
{
	// Simplest function to bind
	*ret = spn_makestring_nocopy(SDL_GetPlatform());

	return 0;
}

int spnlib_SDL_GetCPUSpecs(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makehashmap();
	SpnHashMap *cpu_features = spn_hashmapvalue(ret);

	// These give integers
	SpnValue cache = spn_makeint(SDL_GetCPUCacheLineSize());
	SpnValue cores = spn_makeint(SDL_GetCPUCount());
	SpnValue ram = spn_makeint(SDL_GetSystemRAM());
	// And these give booleans
	SpnValue now3d = SPN_SDLBOOL(SDL_Has3DNow());
	SpnValue avx = SPN_SDLBOOL(SDL_HasAVX());
	SpnValue avx2 = SPN_SDLBOOL(SDL_HasAVX2());
	SpnValue altivec = SPN_SDLBOOL(SDL_HasAltiVec());
	SpnValue mmx = SPN_SDLBOOL(SDL_HasMMX());
	SpnValue rdtsc = SPN_SDLBOOL(SDL_HasRDTSC());
	SpnValue sse = SPN_SDLBOOL(SDL_HasSSE());
	SpnValue sse2 = SPN_SDLBOOL(SDL_HasSSE2());
	SpnValue sse3 = SPN_SDLBOOL(SDL_HasSSE3());
	SpnValue sse41 = SPN_SDLBOOL(SDL_HasSSE41());
	SpnValue sse42 = SPN_SDLBOOL(SDL_HasSSE42());

	// Assembling all this mess
	spn_hashmap_set_strkey(cpu_features, "Cache (KB)", &cache);
	spn_hashmap_set_strkey(cpu_features, "Locical Cores", &cores);
	spn_hashmap_set_strkey(cpu_features, "RAM (MB)", &ram);
	spn_hashmap_set_strkey(cpu_features, "3DNow", &now3d);
	spn_hashmap_set_strkey(cpu_features, "AVX", &avx);
	spn_hashmap_set_strkey(cpu_features, "AVX2", &avx2);
	spn_hashmap_set_strkey(cpu_features, "AltiVec", &altivec);
	spn_hashmap_set_strkey(cpu_features, "MMX", &mmx);
	spn_hashmap_set_strkey(cpu_features, "RDTSC", &rdtsc);
	spn_hashmap_set_strkey(cpu_features, "SSE", &sse);
	spn_hashmap_set_strkey(cpu_features, "SSE2", &sse2);
	spn_hashmap_set_strkey(cpu_features, "SSE3", &sse3);
	spn_hashmap_set_strkey(cpu_features, "SSE41", &sse41);
	spn_hashmap_set_strkey(cpu_features, "SSE42", &sse42);

	spn_value_release(&cache);
	spn_value_release(&cores);
	spn_value_release(&ram);
	spn_value_release(&now3d);
	spn_value_release(&avx);
	spn_value_release(&avx2);
	spn_value_release(&altivec);
	spn_value_release(&mmx);
	spn_value_release(&rdtsc);
	spn_value_release(&sse);
	spn_value_release(&sse2);
	spn_value_release(&sse3);
	spn_value_release(&sse41);
	spn_value_release(&sse42);

	return 0;
}

/////////////////////////////////
////    Power Management    /////
/////////////////////////////////
static const char *powerstate_to_string(SDL_PowerState state)
{
	switch (state) {
		case SDL_POWERSTATE_ON_BATTERY:
			return "not plugged in, running on the battery";
		case SDL_POWERSTATE_NO_BATTERY:
			return "plugged in, no battery available";
		case SDL_POWERSTATE_CHARGING:
			return "plugged in, charging battery";
		case SDL_POWERSTATE_CHARGED:
			return "plugged in, battery charged";
		case SDL_POWERSTATE_UNKNOWN:
		default:
			return "cannot determine power status";
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
