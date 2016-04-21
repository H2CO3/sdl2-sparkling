//
// sdl2_event.c
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 02/04/2015
//
// Licensed under the 2-clause BSD License
//

//
// Helpers for 'event_to_hashmap()'
//

#include "sdl2_event.h"
#include "helpers.h"


//
// Helpers for 'event_to_hashmap()'
//

static SpnValue flags_from_modifier(SDL_Keymod mod)
{
	SpnValue ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(&ret);

	struct {
		const char *name;
		int flag;
	} flags[] = {
		{ "lshift",   mod & KMOD_LSHIFT },
		{ "rshift",   mod & KMOD_RSHIFT },
		{ "shift",    mod & KMOD_SHIFT  }, // either of left or right shift
		{ "lctrl",    mod & KMOD_LCTRL  },
		{ "rctrl",    mod & KMOD_RCTRL  },
		{ "ctrl",     mod & KMOD_CTRL   }, // either of left or right control
		{ "lalt",     mod & KMOD_LALT   },
		{ "ralt",     mod & KMOD_RALT   },
		{ "alt",      mod & KMOD_ALT    }, // either of left or right alt
		{ "lsuper",   mod & KMOD_LGUI   },
		{ "rsuper",   mod & KMOD_RGUI   },
		{ "super",    mod & KMOD_GUI    }, // either of left or right Windows/Command
		{ "numlock",  mod & KMOD_NUM    },
		{ "capslock", mod & KMOD_CAPS   }
	};

	for (size_t i = 0; i < sizeof flags / sizeof flags[0]; i++) {
		spn_hashmap_set_strkey(
			hm,
			flags[i].name,
			&(SpnValue){ .type = SPN_TYPE_BOOL, .v.b = !!flags[i].flag }
		);
	}

	return ret;
}

static const char *get_mouse_button_name(Uint8 button)
{
	switch (button) {
	case SDL_BUTTON_LEFT:   return "left";
	case SDL_BUTTON_RIGHT:  return "right";
	case SDL_BUTTON_MIDDLE: return "middle";
	case SDL_BUTTON_X1:     return "X1";
	case SDL_BUTTON_X2:     return "X2";
	default:                return NULL;
	}
}

static SpnValue flags_from_mouse_button(Uint32 mask)
{
	SpnValue ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(&ret);

	struct {
		const char *name;
		int flag;
	} flags[] = {
		{ "left",   mask & SDL_BUTTON_LMASK  },
		{ "right",  mask & SDL_BUTTON_RMASK  },
		{ "middle", mask & SDL_BUTTON_MMASK  },
		{ "X2",     mask & SDL_BUTTON_X1MASK },
		{ "X2",     mask & SDL_BUTTON_X2MASK }
	};

	for (size_t i = 0; i < sizeof flags / sizeof flags[0]; i++) {
		spn_hashmap_set_strkey(
			hm,
			flags[i].name,
			&(SpnValue){ .type = SPN_TYPE_BOOL, .v.b = !!flags[i].flag }
		);
	}

	return ret;
}

static const char *get_window_event_name(SDL_WindowEventID value)
{
	switch (value) {
	case SDL_WINDOWEVENT_SHOWN:        return "shown";
	case SDL_WINDOWEVENT_HIDDEN:       return "hidden";
	case SDL_WINDOWEVENT_EXPOSED:      return "exposed";
	case SDL_WINDOWEVENT_MOVED:        return "moved";
	case SDL_WINDOWEVENT_RESIZED:      return "resized";
	case SDL_WINDOWEVENT_SIZE_CHANGED: return "size_changed";
	case SDL_WINDOWEVENT_MINIMIZED:    return "minimized";
	case SDL_WINDOWEVENT_MAXIMIZED:    return "maximized";
	case SDL_WINDOWEVENT_RESTORED:     return "restored";
	case SDL_WINDOWEVENT_ENTER:        return "enter";
	case SDL_WINDOWEVENT_LEAVE:        return "leave";
	case SDL_WINDOWEVENT_FOCUS_GAINED: return "focus_gained";
	case SDL_WINDOWEVENT_FOCUS_LOST:   return "focus_lost";
	case SDL_WINDOWEVENT_CLOSE:        return "close";
	default: return NULL;
	}
}

// Converts an SDL_Event structure to an SpnHashMap object
static SpnValue event_to_hashmap(SDL_Event *event)
{
	SpnValue ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(&ret);
	const char *type = "";

	switch (event->type) {
	case SDL_KEYDOWN: // fallthru
	case SDL_KEYUP: {
		type = "keyboard";

		set_string_property(hm, "value", SDL_GetKeyName(event->key.keysym.sym));

		SpnValue mod = flags_from_modifier(event->key.keysym.mod);
		spn_hashmap_set_strkey(hm, "modifier", &mod);
		spn_value_release(&mod);

		SpnValue state = spn_makebool(event->key.state == SDL_PRESSED);
		spn_hashmap_set_strkey(hm, "state", &state);

		break;
	}
	case SDL_MOUSEBUTTONDOWN: // fallthru
	case SDL_MOUSEBUTTONUP: {
		type = "mousebutton";

		const char *btnname = get_mouse_button_name(event->button.button);
		set_string_property_nocopy(hm, "button", btnname);

		SpnValue state = spn_makebool(event->button.state == SDL_PRESSED);
		spn_hashmap_set_strkey(hm, "state", &state);

		set_integer_property(hm, "count", event->button.clicks);
		set_integer_property(hm, "x", event->button.x);
		set_integer_property(hm, "y", event->button.y);

		break;
	}
	case SDL_MOUSEMOTION: {
		type = "mousemove";

		set_integer_property(hm, "x", event->motion.x);
		set_integer_property(hm, "y", event->motion.y);
		set_integer_property(hm, "dx", event->motion.xrel);
		set_integer_property(hm, "dy", event->motion.yrel);

		SpnValue flags = flags_from_mouse_button(event->motion.state);
		spn_hashmap_set_strkey(hm, "buttons", &flags);
		spn_value_release(&flags);

		break;
	}
	case SDL_MOUSEWHEEL:
		type = "mousewheel";

		set_integer_property(hm, "x", event->wheel.x);
		set_integer_property(hm, "y", event->wheel.y);

		break;
	case SDL_FINGERDOWN:   // fallthru
	case SDL_FINGERMOTION: // fallthru
	case SDL_FINGERUP:
		// first off, set type
		switch (event->type) {
		case SDL_FINGERDOWN:   type = "touchdown"; break;
		case SDL_FINGERMOTION: type = "touchmove"; break;
		case SDL_FINGERUP:     type = "touchup"; break;
		}

		// set geometrical properties
		// Note that x, y, dx and dy are normalized
		// to the [0...1] interval, unlike the similar properties
		// of events of other types.
		set_integer_property(hm, "finger", event->tfinger.fingerId);
		set_float_property(hm, "x", event->tfinger.x);
		set_float_property(hm, "y", event->tfinger.y);
		set_float_property(hm, "dx", event->tfinger.dx);
		set_float_property(hm, "dy", event->tfinger.dy);
		set_float_property(hm, "pressure", event->tfinger.pressure);

		break;
	case SDL_MULTIGESTURE:
		type = "gesture";

		// set geometrical properties
		set_integer_property(hm, "fingerCount", event->mgesture.numFingers);

		// again, x, y and distance are normalized to [0...1],
		// and rotation is measured in radians.
		set_float_property(hm, "x", event->mgesture.x);
		set_float_property(hm, "y", event->mgesture.y);
		set_float_property(hm, "rotation", event->mgesture.dTheta);
		set_float_property(hm, "distance", event->mgesture.dDist);

		break;
	case SDL_QUIT:
		type = "quit";
		break;
	case SDL_USEREVENT: {
		type = "timer";
		SpnValue timer = spn_makestrguserinfo(event->user.data1);
		spn_hashmap_set_strkey(hm, "ID", &timer);
		// do _not_ release 'timer' - we do not own it
		break;
	}
	case SDL_WINDOWEVENT: {
		type = "window";
		set_integer_property(hm, "ID", event->window.windowID);

		// set SDL_WindowEventID (SHOWN, HIDDEN, EXPOSED, etc.)
		const char *name = get_window_event_name(event->window.event);
		set_string_property_nocopy(hm, "name", name);

		// set event dependent data
		set_integer_property(hm, "data1", event->window.data1);
		set_integer_property(hm, "data2", event->window.data2);
		break;
	}
	case SDL_DROPFILE:
		type = "drop";
		char *filename = event->drop.file;
		set_string_property(hm, "value", filename);
		SDL_free(filename);
		break;
	default:
		break;
	}

	// set type string and timestamp
	SpnValue typestr = spn_makestring_nocopy(type);
	spn_hashmap_set_strkey(hm, "type", &typestr);
	spn_value_release(&typestr);

	set_integer_property(hm, "timestamp", event->common.timestamp);

	return ret;
}

// Returns the next available event or
// nil if there's no event to process
// Typically called in a loop.
int spnlib_SDL_PollEvent(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	SDL_Event event;
	if (SDL_PollEvent(&event)) {
		*ret = event_to_hashmap(&event);
	}

	// if there's no event to poll, return nil implicitly
	return 0;
}
