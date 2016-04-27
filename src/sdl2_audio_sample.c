//
// sdl2_audio_sample.c
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 21/04/2016
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_audio.h"
#include "sdl2_audio_structs.h"
#include "sdl2_sparkling.h"
#include "helpers.h"

/////////////////////////////////
//   Sample Class structure    //
/////////////////////////////////
static void spn_SDL_Sample_dtor(void *obj)
{
	spn_SDL_Sample *Sample = obj;
	Mix_FreeChunk(Sample->chunk); Sample->chunk = NULL;
	Mix_CloseAudio();
}

const SpnClass spn_SDL_Sample_class = {
	sizeof(spn_SDL_Sample),
	SPN_SDL_CLASS_UID_AUDIO,
	NULL,
	NULL,
	NULL,
	spn_SDL_Sample_dtor
};

spn_SDL_Sample *from_hashmap_grab_Sample(SpnHashMap *hm)
{
	SpnValue objv = spn_hashmap_get_strkey(hm, "sample");

	if (!spn_isstrguserinfo(&objv)) {
		return NULL;
	}

	spn_SDL_Sample *Sample = spn_objvalue(&objv);

	if (!spn_object_member_of_class(Sample, &spn_SDL_Sample_class)) {
		return NULL;
	}

	return Sample;
}


/////////////////////////////////
//   Initialize Sample Class   //
/////////////////////////////////
static SpnValue spnlib_SDL_Sample_new(void)
{
	spn_SDL_Sample *obj = spn_object_new(&spn_SDL_Sample_class);
	obj->chunk = NULL;
	return spn_makestrguserinfo(obj);
}

int spnlib_SDL_OpenSample(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, int);      // freq
	CHECK_ARG_RETURN_ON_ERROR(1, string);   // format
	CHECK_ARG_RETURN_ON_ERROR(2, int);      // channels
	CHECK_ARG_RETURN_ON_ERROR(3, int);      // chunksize

	*ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(ret);

	// set its prototype
	SpnValue proto = spn_get_lib_prototype("Sample");
	spn_hashmap_set_strkey(hm, "super", &proto);

	// prepare values from arguments
	int freq =            INTARG(0);
	SDL_AudioFormat fmt = get_audioformat_value(STRARG(1));
	int channels =        INTARG(2);
	int chunksize =       INTARG(3);

	// create mixer object
	if (Mix_OpenAudio(freq, fmt, channels, chunksize) < 0) {
		// The only time where GetError() is obligatorily called by the library
		const void *args[1] = { Mix_GetError() };
		spn_ctx_runtime_error(ctx, "couldn't initialize SDL_mixer", args);
		return -2;
	}

	// fill mixer properties
	SpnValue sample = spnlib_SDL_Sample_new();
	spn_hashmap_set_strkey(hm, "sample", &sample);
	spn_value_release(&sample);

	fill_audio_hashmap_with_values(hm, chunksize);

	return 0;
}

/////////////////////////////////
//    Mixer's Sample actions   //
/////////////////////////////////
// methods
static int spnlib_SDL_Sample_listDecoders(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makearray();
	SpnArray *arr = spn_arrayvalue(ret);

	set_decoder_list_array(arr, Mix_GetNumChunkDecoders(), Mix_GetChunkDecoder);
	return 0;
}

static int spnlib_SDL_Sample_load(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_FOR_AUDIO_HASHMAP(0, Sample);
	CHECK_ARG_RETURN_ON_ERROR(1, string); // filename

	Sample->chunk = Mix_LoadWAV(STRARG(1));
	*ret = Sample->chunk ? spn_trueval : spn_falseval;

	return 0;
}

static int spnlib_SDL_Sample_volume(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_FOR_AUDIO_HASHMAP(0, Sample);

	Sint8 vol = -1;
	if (argc >= 2) {
		CHECK_ARG_RETURN_ON_ERROR(1, number);
		vol = constrain_to_01(NUMARG(1)) * 128;
	}
	*ret = spn_makefloat(Mix_VolumeChunk(Sample->chunk, vol) / 128.0);

	return 0;
}

/////////////////////////////////
//    Audio methods creation   //
/////////////////////////////////
void spnlib_SDL_methods_for_Sample(SpnHashMap *audio)
{
	static const SpnExtFunc methods[] = {
		{ "listDecoders", spnlib_SDL_Sample_listDecoders },
		{ "load",         spnlib_SDL_Sample_load         },
		{ "volume",       spnlib_SDL_Sample_volume       }
	};

	for (size_t i = 0; i < COUNT(methods); i++) {
		SpnValue fnval = spn_makenativefunc(methods[i].name, methods[i].fn);
		spn_hashmap_set_strkey(audio, methods[i].name, &fnval);
		spn_value_release(&fnval);
	}
}
