//
// sdl2_audio_channels.c
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
//  Channels Class structure   //
/////////////////////////////////
static void spn_SDL_Channels_dtor(void *obj)
{
	Mix_AllocateChannels(0);
}

const SpnClass spn_SDL_Channels_class = {
	sizeof(spn_SDL_Channels),
	SPN_SDL_CLASS_UID_AUDIO,
	NULL,
	NULL,
	NULL,
	spn_SDL_Channels_dtor
};

/////////////////////////////////
//    Initialize Audio Class   //
/////////////////////////////////
static SpnValue spnlib_SDL_Channels_new(int numchannels)
{
	spn_SDL_Channels *obj = spn_object_new(&spn_SDL_Channels_class);
	Mix_AllocateChannels(numchannels);
	return spn_makestrguserinfo(obj);
}

int spnlib_SDL_OpenChannels(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, int);      // numchannels

	*ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(ret);

	// set its prototype
	SpnValue proto = spn_get_lib_prototype("Channels");
	spn_hashmap_set_strkey(hm, "super", &proto);

	// prepare values from arguments
	int numchannels = INTARG(0);

	// fill channels properties
	spnlib_SDL_Channels_new(numchannels);
	set_integer_property(hm, "number", numchannels);

	return 0;
}

/////////////////////////////////
//   Mixer's Channels actions  //
/////////////////////////////////
// auxiliary
CallbackData CB_data;

static void c_callback(int channel)
{
	CallbackData *c_data = &CB_data;

	#define CB_NUMARGS 1
	SpnValue argv[CB_NUMARGS] = { spn_makeint(channel) };
	// FIXME: I need to somehow report the error through SpnContext
	if (spn_ctx_callfunc(c_data->ctx, c_data->fn, NULL, CB_NUMARGS, argv) != 0) {
		spn_ctx_runtime_error(c_data->ctx, "fatal error with callback function", NULL);
	}

	for (int i = 0; i < CB_NUMARGS; i++) {
		spn_value_release(&argv[i]);
	}
	#undef CB_NUMARGS
}

// methods
static int spnlib_SDL_Channels_reallocate(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, int);

	SpnHashMap *hm = HASHMAPARG(0);
	int num = INTARG(1);
	Mix_AllocateChannels(num);
	set_integer_property(hm, "number", num);

	return 0;
}

static int spnlib_SDL_Channels_volume(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int);

	int channel = INTARG(1);
	Sint8 vol = -1;
	if (argc >= 3) {
		CHECK_ARG_RETURN_ON_ERROR(2, number);
		vol = constrain_to_01(NUMARG(2)) * 128;
	}
	*ret = spn_makefloat(Mix_Volume(channel, vol) / 128.0);

	return 0;
}

static int spnlib_SDL_Channels_play(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int);  // channel
	CHECK_FOR_AUDIO_HASHMAP(2, Sample); // sample
	CHECK_ARG_RETURN_ON_ERROR(3, int);  // loops

	int channel = INTARG(1);
	int loops = INTARG(3);
	int ticks = -1;

	if (argc >= 5) {
		CHECK_ARG_RETURN_ON_ERROR(4, int);  // ticks
		ticks = INTARG(4);
	}

	int success = Mix_PlayChannelTimed(channel, Sample->chunk, loops, ticks);
	*ret = success != -1 ? spn_trueval : spn_falseval;
	return 0;
}

static int spnlib_SDL_Channels_fadeIn(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int);  // channel
	CHECK_FOR_AUDIO_HASHMAP(2, Sample); // sample
	CHECK_ARG_RETURN_ON_ERROR(3, int);  // loops
	CHECK_ARG_RETURN_ON_ERROR(4, int);  // fade

	int channel = INTARG(1);
	int loops = INTARG(3);
	int fade = INTARG(4);
	int ticks = -1;

	if (argc >= 6) {
		CHECK_ARG_RETURN_ON_ERROR(5, int);  // ticks
		ticks = INTARG(5);
	}

	int success = Mix_FadeInChannelTimed(channel, Sample->chunk, loops, fade, ticks);
	*ret = success != -1 ? spn_trueval : spn_falseval;
	return 0;
}

static int spnlib_SDL_Channels_fadeOut(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int); // channel
	CHECK_ARG_RETURN_ON_ERROR(2, int); // fade
	Mix_FadeOutChannel(INTARG(1), INTARG(2));
	return 0;
}

static int spnlib_SDL_Channels_pause(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int); // channel
	Mix_Pause(INTARG(1));
	return 0;
}

static int spnlib_SDL_Channels_resume(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int); // channel
	Mix_Resume(INTARG(1));
	return 0;
}

static int spnlib_SDL_Channels_isPlaying(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int); // channel

	int channel = INTARG(1);
	if (channel > 0) {
		*ret = spn_makebool(Mix_Playing(channel));
	} else {
		*ret = spn_makeint(Mix_Playing(-1));
	}

	return 0;
}

static int spnlib_SDL_Channels_isPaused(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int); // channel

	int channel = INTARG(1);
	if (channel > 0) {
		*ret = spn_makebool(Mix_Paused(channel));
	} else {
		*ret = spn_makeint(Mix_Paused(-1));
	}

	return 0;
}

static int spnlib_SDL_Channels_isFading(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int);

	Mix_Fading fade = Mix_FadingChannel(INTARG(1));
	if (fade != MIX_NO_FADING) {
		*ret = spn_makestring_nocopy(fade_to_string(fade));
	}

	return 0;
}

static int spnlib_SDL_Channels_halt(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, int); // channel

	int milliseconds = 0;
	if (argc >= 3) {
		CHECK_ARG_RETURN_ON_ERROR(2, int); // milliseconds
		milliseconds = INTARG(2);
	}
	Mix_ExpireChannel(INTARG(1), milliseconds);

	return 0;
}

static int spnlib_SDL_Channels_finisher(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(1, func);

	CB_data.ctx = ctx;
	CB_data.fn  = FUNCARG(1);
	Mix_ChannelFinished(c_callback);

	return 0;
}

/////////////////////////////////
//    Audio methods creation   //
/////////////////////////////////
void spnlib_SDL_methods_for_Channels(SpnHashMap *audio)
{
	static const SpnExtFunc methods[] = {
		{ "reallocate", spnlib_SDL_Channels_reallocate },
		{ "volume",     spnlib_SDL_Channels_volume     },
		{ "play",       spnlib_SDL_Channels_play       },
		{ "fadeIn",     spnlib_SDL_Channels_fadeIn     },
		{ "fadeOut",    spnlib_SDL_Channels_fadeOut    },
		{ "pause",      spnlib_SDL_Channels_pause      },
		{ "resume",     spnlib_SDL_Channels_resume     },
		{ "isPlaying",  spnlib_SDL_Channels_isPlaying  },
		{ "isPaused",   spnlib_SDL_Channels_isPaused   },
		{ "isFading",   spnlib_SDL_Channels_isFading   },
		{ "halt",       spnlib_SDL_Channels_halt       },
		{ "finisher",   spnlib_SDL_Channels_finisher   }
	};

	for (size_t i = 0; i < COUNT(methods); i++) {
		SpnValue fnval = spn_makenativefunc(methods[i].name, methods[i].fn);
		spn_hashmap_set_strkey(audio, methods[i].name, &fnval);
		spn_value_release(&fnval);
	}
}
