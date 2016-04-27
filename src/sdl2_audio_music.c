//
// sdl2_audio_music.c
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 03/04/2016
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_audio.h"
#include "sdl2_audio_structs.h"
#include "sdl2_sparkling.h"
#include "helpers.h"


/////////////////////////////////
//    Music Class structure    //
/////////////////////////////////
static void spn_SDL_Music_dtor(void *obj)
{
	spn_SDL_Music *Music = obj;
	Mix_FreeMusic(Music->music); Music->music = NULL;
	Mix_CloseAudio();
}

const SpnClass spn_SDL_Music_class = {
	sizeof(spn_SDL_Music),
	SPN_SDL_CLASS_UID_AUDIO,
	NULL,
	NULL,
	NULL,
	spn_SDL_Music_dtor
};

spn_SDL_Music *from_hashmap_grab_Music(SpnHashMap *hm)
{
	SpnValue objv = spn_hashmap_get_strkey(hm, "music");

	if (!spn_isstrguserinfo(&objv)) {
		return NULL;
	}

	spn_SDL_Music *music = spn_objvalue(&objv);

	if (!spn_object_member_of_class(music, &spn_SDL_Music_class)) {
		return NULL;
	}

	return music;
}


/////////////////////////////////
//    Initialize Music Class   //
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
	CHECK_ARG_RETURN_ON_ERROR(3, int);      // chunksize

	*ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(ret);

	// set its prototype
	SpnValue proto = spn_get_lib_prototype("Music");
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
	SpnValue music = spnlib_SDL_Music_new();
	spn_hashmap_set_strkey(hm, "music", &music);
	spn_value_release(&music);

	fill_audio_hashmap_with_values(hm, chunksize);

	return 0;
}

/////////////////////////////////
//    Mixer's Music actions   //
/////////////////////////////////
// auxiliary
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
	CHECK_FOR_AUDIO_HASHMAP(0, Music);
	CHECK_ARG_RETURN_ON_ERROR(1, string); // filename

	Music->music = Mix_LoadMUS(STRARG(1));
	*ret = Music->music ? spn_trueval : spn_falseval;

	return 0;
}

static int spnlib_SDL_Music_getType(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_FOR_AUDIO_HASHMAP(0, Music);
	const char *type = music_type_to_string(Mix_GetMusicType(Music->music));
	*ret = spn_makestring_nocopy(type);
	return 0;
}

static int spnlib_SDL_Music_play(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_FOR_AUDIO_HASHMAP(0, Music);
	CHECK_ARG_RETURN_ON_ERROR(1, int);

	Mix_PlayMusic(Music->music, INTARG(1));
	return 0;
}

static int spnlib_SDL_Music_fadeIn(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_FOR_AUDIO_HASHMAP(0, Music);
	CHECK_ARG_RETURN_ON_ERROR(1, int);
	CHECK_ARG_RETURN_ON_ERROR(2, int);
	if (argc >= 4) {
		CHECK_ARG_RETURN_ON_ERROR(3, int);
	}

	if (argc < 4) {
		Mix_FadeInMusic(Music->music, INTARG(1), INTARG(2));
	} else {
		Mix_FadeInMusicPos(Music->music, INTARG(1), INTARG(2), INTARG(3));
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
	Sint8 vol = -1;
	if (argc >= 2) {
		CHECK_ARG_RETURN_ON_ERROR(1, number);
		vol = constrain_to_01(NUMARG(1)) * 128;
	}
	*ret = spn_makefloat(Mix_VolumeMusic(vol) / 128.0); // bit shifting wouldn't do

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
	CHECK_ARG_RETURN_ON_ERROR(1, number);
	Mix_SetMusicPosition(NUMARG(1));
	return 0;
}

static int spnlib_SDL_Music_halt(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	Mix_HaltMusic();
	return 0;
}

static int spnlib_SDL_Music_fromCMD(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, string);
	Mix_SetMusicCMD(STRARG(1));
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
