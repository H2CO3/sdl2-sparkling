//
// sdl2_audio.c
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 21/04/2016
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_audio.h"
#include "helpers.h"


/////////////////////////////////
//    Audio Class materials    //
/////////////////////////////////
const char *get_audioformat_string(SDL_AudioFormat fmt)
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

SDL_AudioFormat get_audioformat_value(const char *name)
{
	const struct {
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

const char *get_channel_string(Uint8 channel)
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

const char *fade_to_string(Mix_Fading fade)
{
	switch (fade) {
    case MIX_FADING_OUT: return "in";
    case MIX_FADING_IN:  return "out";
	case MIX_NO_FADING:; // FALLTHRU
	}

	SHANT_BE_REACHED();
}

void fill_audio_hashmap_with_values(SpnHashMap *hm, int sample)
{
	int freq, channels;
	SDL_AudioFormat fmt;

	Mix_QuerySpec(&freq, &fmt, &channels);

	set_integer_property(hm, "frequency", freq);
	set_string_property_nocopy(hm, "format", get_audioformat_string(fmt));
	set_string_property_nocopy(hm, "channels", get_channel_string(channels));
	set_integer_property(hm, "frequency", freq);
}

void set_decoder_list_array(
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
