//
// sdl2_audio.c
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 03/04/2016
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_audio.h"
#include "sdl2_sparkling.h"

typedef struct spn_SDL_Audio {
	SpnObject base;
	SDL_AudioDeviceID ID;
	// FIXME: If this structure won't hold any other kind of data, I might as
	// FIXME: well swap it with a simple SpnValue holder for the ID
} spn_SDL_Audio;


static void spn_SDL_Audio_dtor(void *obj)
{
	spn_SDL_Audio *device = obj;
	SDL_CloseAudioDevice(device->ID);
}

// A simple RAII class for managing SDL_Texture objects
const SpnClass spn_SDL_Audio_class = {
	sizeof(spn_SDL_Audio),
	SPN_SDL_CLASS_UID_AUDIO,
	NULL,
	NULL,
	NULL,
	spn_SDL_Audio_dtor
};

// Retrieves an internal audio descriptor from a "public" audio object
spn_SDL_Audio *audio_from_hashmap(SpnHashMap *hm)
{
	SpnValue objv = spn_hashmap_get_strkey(hm, "device");

	if (!spn_isstrguserinfo(&objv)) {
		return NULL;
	}

	spn_SDL_Audio *audio = spn_objvalue(&objv);

	if (!spn_object_member_of_class(audio, &spn_SDL_Audio_class)) {
		return NULL;
	}

	return audio;
}

//
// Audio Class construction materials
//

static const char *get_audioformat_string(SDL_AudioFormat fmt)
{
	switch (fmt) {
	// 8-bit
	case AUDIO_S8:      return "signed 8-bit";
	case AUDIO_U8:      return "unsigned 8-bit";
	// 16-bit
	case AUDIO_S16:     return "signed 16-bit, little-endian byte";
	case AUDIO_S16MSB:  return "signed 16-bit, big-endian byte";

	case AUDIO_U16:     return "unsigned 16-bit, little-endian byte";
	case AUDIO_U16MSB:  return "unsigned 16-bit, big-endian byte";
	// 32-bit
	case AUDIO_S32:     return "32-bit integer samples, little-endian byte";
	case AUDIO_S32MSB:  return "32-bit integer samples, big-endian byte";
	// float
	case AUDIO_F32:     return "32-bit floating point, little-endian byte";
	case AUDIO_F32MSB:  return "32-bit floating point, big-endian byte";

	default:            return "unknown";
	}

	SHANT_BE_REACHED();
}

static SDL_AudioFormat get_audioformat_value(const char *name)
{
	static const struct {
		const char *name;
		SDL_AudioFormat fmt;
	} formats[] = {
		// 8-bit
		{ "S8",     AUDIO_S8     },
		{ "U8",     AUDIO_U8     },
		// 16-bit
		{ "S16",    AUDIO_S16    },
		{ "S16MSB", AUDIO_S16MSB },
		{ "U16",    AUDIO_U16    },
		{ "U16MSB", AUDIO_U16MSB },
		// 32-bit
		{ "S32",    AUDIO_S32    },
		{ "S32MSB", AUDIO_S32MSB },
		// float
		{ "F32",    AUDIO_F32    },
		{ "F32MSB", AUDIO_F32MSB }
	};

	for (size_t i = 0; i < sizeof formats / sizeof formats[0]; i++) {
		if (strcmp(formats[i].name, name) == 0) {
			return formats[i].fmt;
		}
	}

	return 0;
}

// TODO: support callback feature in SDL_AudioSpec
static SDL_AudioSpec get_audiospec_from_arg(SpnArray *arr)
{
	SDL_AudioSpec ret;
	SDL_memset(&ret, 0, sizeof(ret));

	SpnValue val;
	// 1. int freq
	val          = spn_array_get(arr, 0);
	ret.freq     = spn_intvalue_f(&val);
	spn_value_release(&val);
	// 2. SDL_AudioFormat format
	val          = spn_array_get(arr, 1);
	ret.format   = get_audioformat_value(spn_stringvalue(&val)->cstr);
	spn_value_release(&val);
	// 3. Uint8 channels
	val          = spn_array_get(arr, 2);
	ret.channels = spn_intvalue_f(&val);
	spn_value_release(&val);
	// 4. Uint16 samples
	val          = spn_array_get(arr, 3);
	ret.samples  = spn_intvalue_f(&val);
	spn_value_release(&val);

	return ret;
}

static const char *channel_to_string(Uint8 channel)
{
	switch (channel) {
	case 1:  return "mono";
	case 2:  return "stereo";
	case 4:  return "quad";
	case 6:  return "5.1";
	default: return "unknown";
	}

	SHANT_BE_REACHED();
}

static void fill_hashmap_with_obtained_audiospec(SpnHashMap *hm, SDL_AudioSpec *spec)
{
	SpnValue val;

	// 1. int freq
	val = spn_makeint(spec->freq);
	spn_hashmap_set_strkey(hm, "freq", &val);
	spn_value_release(&val);
	// 2. string format
	val = spn_makestring_nocopy(get_audioformat_string(spec->format));
	spn_hashmap_set_strkey(hm, "format", &val);
	spn_value_release(&val);
	// 3. string channels
	val = spn_makestring_nocopy(channel_to_string(spec->channels));
	spn_hashmap_set_strkey(hm, "channel", &val);
	spn_value_release(&val);
	// 4. int samples
	val = spn_makeint(spec->samples);
	spn_hashmap_set_strkey(hm, "sample", &val);
	spn_value_release(&val);
}

//
// Initialize Audio Class
//
// FIXME: As soon as SDL supports recording, `iscapture` shall be considered in
// FIXME: the argv. Until then, `iscapture` = 0
static SpnValue spnlib_SDL_Audio_new(
	const char *name,
	SDL_AudioSpec *want,
	SDL_AudioSpec *have
)
{
	spn_SDL_Audio *obj = spn_object_new(&spn_SDL_Audio_class);
	obj->ID = SDL_OpenAudioDevice(name, 0, want, have, 0);
	return spn_makestrguserinfo(obj);
}

// TODO: support "change" flags (like SDL_AUDIO_ALLOW_FREQUENCY_CHANGE)
int spnlib_SDL_OpenAudioDevice(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, array);      // AudioSpec
	if (argc == 2) {
		CHECK_ARG_RETURN_ON_ERROR(1, string); // Audio device name
	}

	*ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(ret);

	// set its prototype
	SpnValue proto = spn_get_lib_prototype("Audio");
	spn_hashmap_set_strkey(hm, "super", &proto);

	// prepare values from arguments
	SDL_AudioSpec have;
	SDL_AudioSpec want = get_audiospec_from_arg(ARRAYARG(0));
	const char *name = argc == 2 ? STRARG(1) : NULL;

	// create device object
	SpnValue device = spnlib_SDL_Audio_new(name, &want, &have);

	// fill device properties
	spn_hashmap_set_strkey(hm, "device", &device);
	fill_hashmap_with_obtained_audiospec(hm, &have);

	spn_value_release(&device);
	return 0;
}

int spnlib_SDL_ListAudioDevices(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makearray();
	SpnArray *arr = spn_arrayvalue(ret);

	int count = SDL_GetNumAudioDevices(0);
	for (int i = 0; i < count; i++) {
		const char *name = SDL_GetAudioDeviceName(i, 0);
		SpnValue device = spn_makestring_nocopy(name);
		spn_array_push(arr, &device);
	}

	return 0;
}

/////////////////////////////////
//     Audio Device actions    //
/////////////////////////////////

static int spnlib_SDL_Audio_close(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Audio *audio = audio_from_hashmap(hm);

	if (audio == NULL) {
		spn_ctx_runtime_error(ctx, "audio object is invalid", NULL);
		return -1;
	}

	if (audio->ID) {
		SDL_CloseAudioDevice(audio->ID);
		audio->ID = 0;
	}

	return 0;
}

static const char *get_audio_device_status(int device)
{
	switch (SDL_GetAudioDeviceStatus(device)) {
    case SDL_AUDIO_STOPPED: return "stopped\n";
    case SDL_AUDIO_PLAYING: return "playing\n";
    case SDL_AUDIO_PAUSED:  return "paused\n";
    default:                return "unknown\n";
    }
}

static int spnlib_SDL_Audio_getStatus(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Audio *audio = audio_from_hashmap(hm);

	if (audio == NULL) {
		spn_ctx_runtime_error(ctx, "audio object is invalid", NULL);
		return -1;
	}

	*ret = spn_makestring_nocopy(get_audio_device_status(audio->ID));
	return 0;
}

//
// Audio methods hashmap creation
//

void spnlib_SDL_methods_for_Audio(SpnHashMap *audio)
{
	static const SpnExtFunc methods[] = {
		{ "close",     spnlib_SDL_Audio_close     },
		{ "getStatus", spnlib_SDL_Audio_getStatus }
	};

	for (size_t i = 0; i < sizeof methods / sizeof methods[0]; i++) {
		SpnValue fnval = spn_makenativefunc(methods[i].name, methods[i].fn);
		spn_hashmap_set_strkey(audio, methods[i].name, &fnval);
		spn_value_release(&fnval);
	}
}
