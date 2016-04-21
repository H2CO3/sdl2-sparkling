//
// sdl2/audio_music.c
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 03/04/2016
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_audio.h"
#include "sdl2_sparkling.h"
#include "helpers.h"


/////////////////////////////////
//  SDL Music class structure  //
/////////////////////////////////
typedef struct spn_SDL_Music {
	SpnObject base;
	Mix_Music *music;
} spn_SDL_Music;

static void spn_SDL_Music_dtor(void *obj)
{
	spn_SDL_Music *mixer = obj;
	Mix_FreeMusic(mixer->music); mixer->music = NULL;
	Mix_CloseAudio();
}

// A simple RAII class for managing SDL_Audio objects
const SpnClass spn_SDL_Music_class = {
	sizeof(spn_SDL_Music),
	SPN_SDL_CLASS_UID_AUDIO,
	NULL,
	NULL,
	NULL,
	spn_SDL_Music_dtor
};

// Retrieves an internal audio descriptor from a "public" audio object
spn_SDL_Music *audio_from_hashmap(SpnHashMap *hm)
{
	SpnValue objv = spn_hashmap_get_strkey(hm, "mixer");

	if (!spn_isstrguserinfo(&objv)) {
		return NULL;
	}

	spn_SDL_Music *mixer = spn_objvalue(&objv);

	if (!spn_object_member_of_class(mixer, &spn_SDL_Music_class)) {
		return NULL;
	}

	return mixer;
}

// A couple lifesavers
#define CHECK_FOR_AUDIO_HASHMAP()                                     \
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);                            \
	SpnHashMap *hm = HASHMAPARG(0);                                   \
	spn_SDL_Music *mixer = audio_from_hashmap(hm);                    \
	if (mixer == NULL) {                                              \
		spn_ctx_runtime_error(ctx, "audio object is invalid", NULL);  \
		return -1;                                                    \
	}

/////////////////////////////////
//    Audio Class materials    //
/////////////////////////////////
static const char *get_audioformat_string(SDL_AudioFormat fmt)
{
	switch (fmt) {
	// 8-bit
	case AUDIO_S8:      return "signed 8-bit";
	case AUDIO_U8:      return "unsigned 8-bit";
	// 16-bit
	case AUDIO_S16LSB:  return "signed 16-bit, little-endian byte";
	case AUDIO_S16MSB:  return "signed 16-bit, big-endian byte";

	case AUDIO_U16LSB:  return "unsigned 16-bit, little-endian byte";
	case AUDIO_U16MSB:  return "unsigned 16-bit, big-endian byte";
	// 32-bit
	case AUDIO_S32LSB:  return "32-bit integer samples, little-endian byte";
	case AUDIO_S32MSB:  return "32-bit integer samples, big-endian byte";
	// float
	case AUDIO_F32LSB:  return "32-bit floating point, little-endian byte";
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
		{ "S16LSB", AUDIO_S16LSB },
		{ "S16MSB", AUDIO_S16MSB },
		{ "S16SYS", AUDIO_S16SYS },
		{ "U16",    AUDIO_U16    },
		{ "U16LSB", AUDIO_U16LSB },
		{ "U16MSB", AUDIO_U16MSB },
		{ "U16SYS", AUDIO_U16SYS },
		// 32-bit
		{ "S32",    AUDIO_S32    },
		{ "S32LSB", AUDIO_S32LSB },
		{ "S32MSB", AUDIO_S32MSB },
		{ "S32SYS", AUDIO_S32SYS },
		// float
		{ "F32",    AUDIO_F32    },
		{ "F32LSB", AUDIO_F32LSB },
		{ "F32MSB", AUDIO_F32MSB },
		{ "F32SYS", AUDIO_F32SYS }
	};

	for (size_t i = 0; i < COUNT(formats); i++) {
		if (strcmp(formats[i].name, name) == 0) {
			return formats[i].fmt;
		}
	}

	SHANT_BE_REACHED();
}

static const char *get_channel_string(Uint8 channel)
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

static void fill_hashmap_with_values(SpnHashMap *hm, int sample)
{
	int freq, channels;
	SDL_AudioFormat fmt;

	Mix_QuerySpec(&freq, &fmt, &channels);

	set_integer_property(hm, "frequency", freq);
	set_string_property_nocopy(hm, "format", get_audioformat_string(fmt));
	set_string_property_nocopy(hm, "channels", get_channel_string(channels));
	set_integer_property(hm, "frequency", freq);
}

/////////////////////////////////
//    Initialize Audio Class   //
/////////////////////////////////
static SpnValue spnlib_SDL_Music_new(void)
{
	spn_SDL_Music *obj = spn_object_new(&spn_SDL_Music_class);
	obj->music = NULL;
	return spn_makestrguserinfo(obj);
}

int spnlib_SDL_OpenMusic(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, int);      // freq
	CHECK_ARG_RETURN_ON_ERROR(1, string);   // format
	CHECK_ARG_RETURN_ON_ERROR(2, int);      // channels
	CHECK_ARG_RETURN_ON_ERROR(3, int);      // sample

	*ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(ret);

	// set its prototype
	SpnValue proto = spn_get_lib_prototype("Music");
	spn_hashmap_set_strkey(hm, "super", &proto);

	// prepare values from arguments
	int freq =            INTARG(0);
	SDL_AudioFormat fmt = get_audioformat_value(STRARG(1));
	int channels =        INTARG(2);
	int sample =          INTARG(3);

	// create mixer object
	if (Mix_OpenAudio(freq, fmt, channels, sample) < 0) {
		// The only time where GetError() is obligatorily called by the library
		const void *args[1] = { Mix_GetError() };
		spn_ctx_runtime_error(ctx, "couldn't initialize SDL_mixer", args);
		return -2;
	}

	// fill mixer properties
	SpnValue mixer = spnlib_SDL_Music_new();
	spn_hashmap_set_strkey(hm, "mixer", &mixer);
	spn_value_release(&mixer);

	fill_hashmap_with_values(hm, sample);

	return 0;
}

/////////////////////////////////
//    Mixer's Music actions   //
/////////////////////////////////
// auxiliary
static void set_decoder_list_array(
	SpnArray *arr,
	size_t count,
	const char *(*GetDecoder)(int)
)
{
	for (size_t i = 0; i < count; i++) {
		SpnValue val = spn_makestring(GetDecoder(i));
		spn_array_push(arr, &val);
		spn_value_release(&val);
	}
}

static const char *music_type_to_string(Mix_MusicType type)
{
	switch (type) {
	case MUS_CMD:  return "command-based";
	case MUS_WAV:  return "wave/riff";
	case MUS_MOD:  return "mod";
	case MUS_MID:  return "midi";
	case MUS_OGG:  return "ogg";
	case MUS_MP3:  return "mp3";
	case MUS_NONE: return "none";
	default:       return "unknown";
	}

	SHANT_BE_REACHED();
}

static const char *fade_to_string(Mix_Fading fade)
{
	switch (fade) {
    case MIX_FADING_OUT: return "in";
    case MIX_FADING_IN:  return "out";
	case MIX_NO_FADING:; // FALLTHRU
	}

	SHANT_BE_REACHED();
}

// methods
static int spnlib_SDL_Music_listDecoders(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makearray();
	SpnArray *arr = spn_arrayvalue(ret);

	set_decoder_list_array(arr, Mix_GetNumMusicDecoders(), Mix_GetMusicDecoder);

	return 0;
}

static int spnlib_SDL_Music_load(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_FOR_AUDIO_HASHMAP();
	CHECK_ARG_RETURN_ON_ERROR(1, string); // filename

	mixer->music = Mix_LoadMUS(STRARG(1));
	*ret = mixer->music ? spn_trueval : spn_falseval;

	return 0;
}

static int spnlib_SDL_Music_getType(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_FOR_AUDIO_HASHMAP();
	const char *type = music_type_to_string(Mix_GetMusicType(mixer->music));
	*ret = spn_makestring_nocopy(type);
	return 0;
}

static int spnlib_SDL_Music_play(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_FOR_AUDIO_HASHMAP();
	CHECK_ARG_RETURN_ON_ERROR(1, int);

	Mix_PlayMusic(mixer->music, INTARG(1));
	return 0;
}

static int spnlib_SDL_Music_fadeIn(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_FOR_AUDIO_HASHMAP();
	CHECK_ARG_RETURN_ON_ERROR(1, int);
	CHECK_ARG_RETURN_ON_ERROR(2, int);
	if (argc == 4) {
		CHECK_ARG_RETURN_ON_ERROR(3, int);
	}

	if (argc < 4) {
		Mix_FadeInMusic(mixer->music, INTARG(1), INTARG(2));
	} else {
		Mix_FadeInMusicPos(mixer->music, INTARG(1), INTARG(2), INTARG(3));
	}
	return 0;
}

static int spnlib_SDL_Music_fadeOut(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, int);
	Mix_FadeOutMusic(INTARG(0));
	return 0;
}

static int spnlib_SDL_Music_volume(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	double vol = -1;
	if (argc == 1) {
		CHECK_ARG_RETURN_ON_ERROR(0, number);
		vol = constrain_to_01(NUMARG(0)) * 128;
	}
	*ret = spn_makefloat(Mix_VolumeMusic(vol) / 128); // bit shifting wouldn't do

	return 0;
}

static int spnlib_SDL_Music_pause(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	Mix_PauseMusic();
	return 0;
}

static int spnlib_SDL_Music_resume(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	Mix_ResumeMusic();
	return 0;
}

static int spnlib_SDL_Music_rewind(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	Mix_RewindMusic();
	return 0;
}

static int spnlib_SDL_Music_isPlaying(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makebool(Mix_PlayingMusic() == 1);
	return 0;
}

static int spnlib_SDL_Music_isPaused(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	*ret = spn_makebool(Mix_PausedMusic() == 1);
	return 0;
}

static int spnlib_SDL_Music_isFading(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	Mix_Fading fade = Mix_FadingMusic();
	if (fade != MIX_NO_FADING) {
		*ret = spn_makestring_nocopy(fade_to_string(fade));
	}
	return 0;
}

static int spnlib_SDL_Music_setPosition(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, number);
	Mix_SetMusicPosition(NUMARG(0));
	return 0;
}

static int spnlib_SDL_Music_halt(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	Mix_HaltMusic();
	return 0;
}

static int spnlib_SDL_Music_fromCMD(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, string);
	Mix_SetMusicCMD(STRARG(0));
	return 0;
}

/////////////////////////////////
//    Audio methods creation   //
/////////////////////////////////
void spnlib_SDL_methods_for_Music(SpnHashMap *audio)
{
	static const SpnExtFunc methods[] = {
		{ "listDecoders", spnlib_SDL_Music_listDecoders },
		{ "load",         spnlib_SDL_Music_load         },
		{ "getType",      spnlib_SDL_Music_getType      },
		{ "play",         spnlib_SDL_Music_play         },
		{ "fadeIn",       spnlib_SDL_Music_fadeIn       },
		{ "fadeOut",      spnlib_SDL_Music_fadeOut      },
		{ "volume",       spnlib_SDL_Music_volume       },
		{ "pause",        spnlib_SDL_Music_pause        },
		{ "resume",       spnlib_SDL_Music_resume       },
		{ "rewind",       spnlib_SDL_Music_rewind       },
		{ "isPlaying",    spnlib_SDL_Music_isPlaying    },
		{ "isPaused",     spnlib_SDL_Music_isPaused     },
		{ "isFading",     spnlib_SDL_Music_isFading     },
		{ "setPosition",  spnlib_SDL_Music_setPosition  },
		{ "halt",         spnlib_SDL_Music_halt         },
		{ "fromCMD",      spnlib_SDL_Music_fromCMD      }
	};

	for (size_t i = 0; i < COUNT(methods); i++) {
		SpnValue fnval = spn_makenativefunc(methods[i].name, methods[i].fn);
		spn_hashmap_set_strkey(audio, methods[i].name, &fnval);
		spn_value_release(&fnval);
	}
}
