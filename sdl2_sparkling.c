//
// sdl2_sparkling.c
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 28/03/2015
//
// Licensed under the 2-clause BSD License
//

#define USE_DYNAMIC_LOADING 1

#include <assert.h>
#include <stdbool.h>

#include <spn/ctx.h>
#include <spn/str.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include "sdl2_sparkling.h"
#include "ttf_support.h"

/////////////////////////////////
// The returned library object //
/////////////////////////////////
static SpnHashMap *library = NULL;

enum {
	SPN_SDL_CLASS_UID_BASE = ('S' << 16) | ('D' << 8) | ('L' << 0),
	SPN_SDL_CLASS_UID_WINDOW = SPN_USER_CLASS_UID_BASE + SPN_SDL_CLASS_UID_BASE + 1,
	SPN_SDL_CLASS_UID_TIMER  = SPN_USER_CLASS_UID_BASE + SPN_SDL_CLASS_UID_BASE + 2
};

////////////////////////////////////////
//          The window class          //
////////////////////////////////////////

typedef struct spn_SDL_Window {
	SpnObject base;
	SDL_Window *window;
	SDL_Renderer *renderer;
	TTF_Font *font;
} spn_SDL_Window;


static void spn_SDL_Window_dtor(void *o)
{
	spn_SDL_Window *obj = o;
	SDL_DestroyWindow(obj->window);
	SDL_DestroyRenderer(obj->renderer);
}

static const SpnClass spn_SDL_Window_class = {
	sizeof(spn_SDL_Window),
	SPN_SDL_CLASS_UID_WINDOW,
	NULL,
	NULL,
	NULL,
	spn_SDL_Window_dtor
};


// Helper for OpenWindow
static SpnValue spn_SDL_Window_new(const char *title, int *width, int *height)
{
	SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	if (*width < 0 || *height < 0) {
		SDL_DisplayMode mode;
		SDL_GetDesktopDisplayMode(0, &mode);
		*width = mode.w;
		*height = mode.h;
		windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	spn_SDL_Window *obj = spn_object_new(&spn_SDL_Window_class);
	obj->window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		*width,
		*height,
		windowFlags
	);

	obj->renderer = SDL_CreateRenderer(
		obj->window,
		-1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
	);

	return spn_makestrguserinfo(obj);
}

// Constructor for window objects.
static int spn_SDL_OpenWindow(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, string);

	// construct return value, a window+renderer object
	*ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(ret);

	// set its prototype
	SpnValue proto = spn_hashmap_get_strkey(library, "Window");
	spn_hashmap_set_strkey(hm, "super", &proto);

	// actually open window
	int width = -1, height = -1;
	if (argc >= 3 && spn_isnumber(&argv[1]) && spn_isnumber(&argv[2])) {
		width = NUMARG(1);
		height = NUMARG(2);
	}

	SpnValue window = spn_SDL_Window_new(STRARG(0), &width, &height);

	// set its properties
	spn_hashmap_set_strkey(hm, "window", &window);
	spn_hashmap_set_strkey(hm, "width",  &(SpnValue){ .type = SPN_TYPE_INT, .v.i = width  });
	spn_hashmap_set_strkey(hm, "height", &(SpnValue){ .type = SPN_TYPE_INT, .v.i = height });

	// handle ownership
	spn_value_release(&window);

	return 0;
}

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

static void set_integer_property(SpnHashMap *hm, const char *name, long n)
{
	SpnValue val = spn_makeint(n);
	spn_hashmap_set_strkey(hm, name, &val);
}

static void set_float_property(SpnHashMap *hm, const char *name, double x)
{
	SpnValue val = spn_makefloat(x);
	spn_hashmap_set_strkey(hm, name, &val);
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

		SpnValue str = spn_makestring(SDL_GetKeyName(event->key.keysym.sym));
		spn_hashmap_set_strkey(hm, "value", &str);
		spn_value_release(&str);

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

		SpnValue btnname = spn_makestring_nocopy(get_mouse_button_name(event->button.button));
		spn_hashmap_set_strkey(hm, "button", &btnname);
		spn_value_release(&btnname);

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

		// set SDL_WindowEventID (SHOWN, HIDDEN, EXPOSED, etc)
		set_integer_property(hm, "event", event->window.event);

		// set event dependent data
		set_integer_property(hm, "data1", event->window.data1);
		set_integer_property(hm, "data2", event->window.data2);
		break;
	}
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
static int spn_SDL_PollEvent(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	SDL_Event event;
	if (SDL_PollEvent(&event)) {
		*ret = event_to_hashmap(&event);
	}

	// if there's no event to poll, return nil implicitly
	return 0;
}

///////////////////////////////////
//////////    Timers    ///////////
///////////////////////////////////

typedef struct spn_SDL_Timer {
	SpnObject base;
	SDL_TimerID ID;
	SpnContext *ctx;
	SpnFunction *callback;
} spn_SDL_Timer;

// If a timer object goes out of scope, stop it
static void spn_SDL_Timer_dtor(void *obj)
{
	spn_SDL_Timer *timer = obj;

	if (timer->ID) {
		SDL_RemoveTimer(timer->ID);
		timer->ID = 0;
	}
}

static const SpnClass spn_SDL_Timer_class = {
	sizeof(spn_SDL_Timer),
	SPN_SDL_CLASS_UID_TIMER,
	NULL,
	NULL,
	NULL,
	spn_SDL_Timer_dtor
};

static Uint32 native_timer_callback(Uint32 delay, void *data)
{
	spn_SDL_Timer *timer = data;

	// if the user supplied a callback function, call it
	// otherwise, just generate a timer event
	if (timer->callback) {
		// XXX: TODO: how can we do error checking here...!?
		spn_ctx_callfunc(timer->ctx, timer->callback, NULL, 0, NULL);
	} else {
		SDL_Event event;
		SDL_zero(event);
		event.type = SDL_USEREVENT;
		event.user.data1 = timer;
		SDL_PushEvent(&event);
	}

	return delay;
}

// Initiate a timer and return the corresponding descriptor object.
// Times are recurring: you must explicitly stop them if you don't
// want them to fire continuously.
// Also, when a timer descriptr object is deallocated (goes out of
// scope), its corresponding timer will be stopped automatically.
// Consequently, in order to keep the timer alive, you need to
// hold a reference to the returned timer descriptor (e. g. store
// it in a variable or a data structure).
// The first argument is the timeout in seconds (may be fractional),
// the second argument is an optional callback function which will
// be called in the current context. If omitted, the timer will
// generate SDL_Events of type "timer" instead.
static int spn_SDL_StartTimer(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, number);

	if (argc > 1) {
		CHECK_ARG_RETURN_ON_ERROR(1, func);
	}

	double dt = NUMARG(0);
	SpnFunction *callback = argc > 1 ? FUNCARG(1) : NULL;

	spn_SDL_Timer *timer = spn_object_new(&spn_SDL_Timer_class);
	SDL_TimerID timerID = SDL_AddTimer(dt * 1000, native_timer_callback, timer);

	timer->ID = timerID;
	timer->ctx = ctx;
	timer->callback = callback;

	*ret = spn_makestrguserinfo(timer);
	return 0;
}

// Stop a timer object.
static int spn_SDL_StopTimer(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, strguserinfo);
	spn_SDL_Timer *timer = OBJARG(0);

	if (timer->base.isa->UID != SPN_SDL_CLASS_UID_TIMER) {
		spn_ctx_runtime_error(ctx, "argument is not a timer object", NULL);
		return -1;
	}

	if (timer->ID) {
		SDL_RemoveTimer(timer->ID);
		timer->ID = 0;
	}

	return 0;
}

// Retrieves an internal window descriptor from
// a "public" window object
spn_SDL_Window *window_from_hashmap(SpnHashMap *hm)
{
	SpnValue objv = spn_hashmap_get_strkey(hm, "window");

	if (!spn_isstrguserinfo(&objv)) {
		return NULL;
	}

	spn_SDL_Window *window = spn_objvalue(&objv);

	if (window->base.isa->UID != SPN_SDL_CLASS_UID_WINDOW) {
		return NULL;
	}

	return window;
}

// Dump ye ole video buffer!
static int spn_SDL_Window_refresh(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;
	SDL_RenderPresent(renderer);

	return 0;
}

// fill the entire window with the current drawing color
static int spn_SDL_Window_clear(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;
	SDL_RenderClear(renderer);

	return 0;
}

// Constrain a floating-point value to the [0...1] closed interval.
static double constrain_to_01(double x)
{
	if (x < 0) {
		return 0;
	}

	if (x > 1) {
		return 1;
	}

	return x;
}

// Set the drawing color in RGBA format.
// Color components are expected to be floating-point values
// in the [0...1] closed interval.
static int spn_SDL_Window_setColor(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, number);
	CHECK_ARG_RETURN_ON_ERROR(2, number);
	CHECK_ARG_RETURN_ON_ERROR(3, number);
	CHECK_ARG_RETURN_ON_ERROR(4, number);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	double r = constrain_to_01(NUMARG(1));
	double g = constrain_to_01(NUMARG(2));
	double b = constrain_to_01(NUMARG(3));
	double a = constrain_to_01(NUMARG(4));

	SDL_SetRenderDrawColor(renderer, r * 255, g * 255, b * 255, a * 255);

	return 0;
}

// Returns a hashmap with keys "r", "g", "b", "a"
// Values are floting-point numbers, normalized to [0...1]
static int spn_SDL_Window_getColor(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	Uint8 r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

	*ret = spn_makehashmap();
	SpnHashMap *color = spn_hashmapvalue(ret);

	SpnValue rv = spn_makefloat(r / 255.0);
	SpnValue gv = spn_makefloat(g / 255.0);
	SpnValue bv = spn_makefloat(b / 255.0);
	SpnValue av = spn_makefloat(a / 255.0);

	spn_hashmap_set_strkey(color, "r", &rv);
	spn_hashmap_set_strkey(color, "g", &gv);
	spn_hashmap_set_strkey(color, "b", &bv);
	spn_hashmap_set_strkey(color, "a", &av);

	return 0;
}

// Parameters:
// 0. window object
// 1. font name (used to construct filename by appeding ".ttf")
// 2. font size in points (72pt = 1 inch)
// 3. font style string ("bold", "italic", "underline", "striketrough", "normal"
//    or any space-spearated combination thereof.)
static int spn_SDL_Window_setFont(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, string);
	CHECK_ARG_RETURN_ON_ERROR(2, number);
	CHECK_ARG_RETURN_ON_ERROR(3, string);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	const char *fontname = STRARG(1);
	int ptsize = NUMARG(2);
	const char *style = STRARG(3);

	window->font = spnlib_sdl2_get_font(fontname, ptsize, style);

	return 0;
}

// Draw a rectangle with coordinates (x, y) and size (w, h).
// if 'fill' is nonzero, fill it with the drawing color,
// otherwise draw the contours only.
static int spn_SDL_Window_drawRect(
	SpnValue *ret,
	int argc,
	SpnValue *argv,
	void *ctx,
	int fill
)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, number);
	CHECK_ARG_RETURN_ON_ERROR(2, number);
	CHECK_ARG_RETURN_ON_ERROR(3, number);
	CHECK_ARG_RETURN_ON_ERROR(4, number);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	if (fill) {
		SDL_RenderFillRect(renderer, &(SDL_Rect){ NUMARG(1), NUMARG(2), NUMARG(3), NUMARG(4) });
	} else {
		SDL_RenderDrawRect(renderer, &(SDL_Rect){ NUMARG(1), NUMARG(2), NUMARG(3), NUMARG(4) });
	}

	return 0;
}

static int spn_SDL_Window_strokeRect(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spn_SDL_Window_drawRect(ret, argc, argv, ctx, 0);
}

static int spn_SDL_Window_fillRect(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spn_SDL_Window_drawRect(ret, argc, argv, ctx, 1);
}

// Draw arc with center (x, y), radius r
// 'start' and 'end' are the staring and ending angle of the
// outline of the arc, measured in radians.
// 'fill' means the same thing as above.
static int spn_SDL_Window_drawArc(
	SpnValue *ret,
	int argc,
	SpnValue *argv,
	void *ctx,
	int fill
)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);

	CHECK_ARG_RETURN_ON_ERROR(1, number); // x
	CHECK_ARG_RETURN_ON_ERROR(2, number); // y
	CHECK_ARG_RETURN_ON_ERROR(3, number); // r
	CHECK_ARG_RETURN_ON_ERROR(4, number); // start
	CHECK_ARG_RETURN_ON_ERROR(5, number); // end

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	double x = NUMARG(1);
	double y = NUMARG(2);
	double r = NUMARG(3);
	double start_r = NUMARG(4);
	double end_r = NUMARG(5);

	Sint16 start = start_r / M_PI * 180;
	Sint16 end = end_r / M_PI * 180;

	Uint8 R, G, B, A;
	SDL_GetRenderDrawColor(renderer, &R, &G, &B, &A);

	// This is necessary because if e. g. start = 0 and end = 2 PI,
	// then sdl2_gfx won't draw *anything* at all.
	int is_full_circle = abs(end - start) >= 2 * M_PI;

	if (fill) {
		if (is_full_circle) {
			filledCircleRGBA(renderer, x, y, r, R, G, B, A);
		} else {
			filledPieRGBA(renderer, x, y, r, start, end, R, G, B, A);
		}
	} else {
		if (is_full_circle) {
			circleRGBA(renderer, x, y, r, R, G, B, A);
		} else {
			arcRGBA(renderer, x, y, r, start, end, R, G, B, A);
		}
	}

	return 0;
}

static int spn_SDL_Window_strokeArc(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spn_SDL_Window_drawArc(ret, argc, argv, ctx, 0);
}

static int spn_SDL_Window_fillArc(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spn_SDL_Window_drawArc(ret, argc, argv, ctx, 1);
}

// Draw an ellipse with center (x, y) and
// horizontal semi-axis rx, veritcal semi-axis ry
static int spn_SDL_Window_drawEllipse(
	SpnValue *ret,
	int argc,
	SpnValue *argv,
	void *ctx,
	int fill
)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);

	CHECK_ARG_RETURN_ON_ERROR(1, number); // x
	CHECK_ARG_RETURN_ON_ERROR(2, number); // y
	CHECK_ARG_RETURN_ON_ERROR(3, number); // rx
	CHECK_ARG_RETURN_ON_ERROR(4, number); // ry

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	double x = NUMARG(1);
	double y = NUMARG(2);
	double rx = NUMARG(3);
	double ry = NUMARG(4);

	Uint8 R, G, B, A;
	SDL_GetRenderDrawColor(renderer, &R, &G, &B, &A);

	if (fill) {
		filledEllipseRGBA(renderer, x, y, rx, ry, R, G, B, A);
	} else {
		ellipseRGBA(renderer, x, y, rx, ry, R, G, B, A);
	}

	return 0;
}

static int spn_SDL_Window_strokeEllipse(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spn_SDL_Window_drawEllipse(ret, argc, argv, ctx, 0);
}

static int spn_SDL_Window_fillEllipse(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spn_SDL_Window_drawEllipse(ret, argc, argv, ctx, 1);
}

// Fill the polygon enclosed by the points (x1, y1), (x2, y2), (x3, y3), ...
// At least 3 points must be specified.
static int spn_SDL_Window_fillPolygon(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	int first_coord_index = 1;
	int ncoords = argc - first_coord_index;

	if (ncoords % 2 != 0) {
		spn_ctx_runtime_error(ctx, "you must supply pairs of coordinates", NULL);
		return -2;
	}

	int npoints = ncoords / 2;
	if (npoints < 3) {
		spn_ctx_runtime_error(ctx, "you must specify at least 3 points", NULL);
		return -3;
	}

	Sint16 vx[npoints];
	Sint16 vy[npoints];

	for (int i = 0; i < ncoords; i += 2) {
		CHECK_ARG_RETURN_ON_ERROR(first_coord_index + i, number);
		CHECK_ARG_RETURN_ON_ERROR(first_coord_index + i + 1, number);
		vx[i / 2] = NUMARG(first_coord_index + i);
		vy[i / 2] = NUMARG(first_coord_index + i + 1);
	}

	Uint8 R, G, B, A;
	SDL_GetRenderDrawColor(renderer, &R, &G, &B, &A);

	filledPolygonRGBA(renderer, vx, vy, npoints, R, G, B, A);

	return 0;
}

// Stroke or fill rounded rectangle at point (x, y) of size (w, h)
// with corner radius r
static int spn_SDL_Window_drawRoundedRect(
	SpnValue *ret,
	int argc,
	SpnValue *argv,
	void *ctx,
	int fill
)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);

	CHECK_ARG_RETURN_ON_ERROR(1, number); // x
	CHECK_ARG_RETURN_ON_ERROR(2, number); // y
	CHECK_ARG_RETURN_ON_ERROR(3, number); // w
	CHECK_ARG_RETURN_ON_ERROR(4, number); // h
	CHECK_ARG_RETURN_ON_ERROR(5, number); // r

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	double x = NUMARG(1);
	double y = NUMARG(2);
	double w = NUMARG(3);
	double h = NUMARG(4);
	double r = NUMARG(5);

	Uint8 R, G, B, A;
	SDL_GetRenderDrawColor(renderer, &R, &G, &B, &A);

	if (fill) {
		roundedBoxRGBA(renderer, x, y, x + w, y + h, r, R, G, B, A);
	} else {
		roundedRectangleRGBA(renderer, x, y, x + w, y + h, r, R, G, B, A);
	}

	return 0;
}

static int spn_SDL_Window_strokeRoundedRect(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spn_SDL_Window_drawRoundedRect(ret, argc, argv, ctx, 0);
}

static int spn_SDL_Window_fillRoundedRect(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spn_SDL_Window_drawRoundedRect(ret, argc, argv, ctx, 1);
}

// Join in s steps the points (x1, y1), (x2, y2), (x3, y3), ...
// with a Bezier curve. 's', the number of steps determines how
// fine the resolution of the curve is (i. e., how close it is to a
// real curve - while it's just a line composed of straight segments)
static int spn_SDL_Window_bezier(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, int);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	int steps = INTARG(1);
	if (steps < 2) {
		spn_ctx_runtime_error(ctx, "you must specify at least 2 interpolation steps", NULL);
		return -2;
	}

	int first_coord_index = 2;
	int ncoords = argc - first_coord_index;

	if (ncoords % 2 != 0) {
		spn_ctx_runtime_error(ctx, "you must supply pairs of coordinates", NULL);
		return -3;
	}

	int npoints = ncoords / 2;
	if (npoints < 3) {
		spn_ctx_runtime_error(ctx, "you must specify at least 3 points", NULL);
		return -4;
	}

	Sint16 vx[npoints];
	Sint16 vy[npoints];

	for (int i = 0; i < ncoords; i += 2) {
		CHECK_ARG_RETURN_ON_ERROR(first_coord_index + i, number);
		CHECK_ARG_RETURN_ON_ERROR(first_coord_index + i + 1, number);
		vx[i / 2] = NUMARG(first_coord_index + i);
		vy[i / 2] = NUMARG(first_coord_index + i + 1);
	}

	Uint8 R, G, B, A;
	SDL_GetRenderDrawColor(renderer, &R, &G, &B, &A);

	bezierRGBA(renderer, vx, vy, npoints, steps, R, G, B, A);

	return 0;
}

// Draw a straight 1px line between points (x, y) and (x + dx, y + dy)
// using the current drawing color.
static int spn_SDL_Window_line(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, number); // x
	CHECK_ARG_RETURN_ON_ERROR(2, number); // y
	CHECK_ARG_RETURN_ON_ERROR(3, number); // dx
	CHECK_ARG_RETURN_ON_ERROR(4, number); // dy

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	double x = NUMARG(1);
	double y = NUMARG(2);
	double dx = NUMARG(3);
	double dy = NUMARG(4);

	SDL_Renderer *renderer = window->renderer;
	SDL_RenderDrawLine(renderer, x, y, x + dx, y + dy);

	return 0;
}

// Set the pixel at point (x, y) to the current drawing color.
static int spn_SDL_Window_point(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, number); // x
	CHECK_ARG_RETURN_ON_ERROR(2, number); // y

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	SDL_RenderDrawPoint(renderer, NUMARG(1), NUMARG(2));

	return 0;
}

// Draw 'text' starting at point (x, y) with the current font.
static int spn_SDL_Window_renderText(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, number); // x
	CHECK_ARG_RETURN_ON_ERROR(2, number); // y
	CHECK_ARG_RETURN_ON_ERROR(3, string); // text
	CHECK_ARG_RETURN_ON_ERROR(4, bool);   // rendering is high-quality?

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	double x = NUMARG(1);
	double y = NUMARG(2);
	const char *text = STRARG(3);
	bool hq = BOOLARG(4);

	spnlib_sdl2_render_text(
		renderer,
		x,
		y,
		text,
		window->font,
		hq
	);

	return 0;
}

// parameters:
// 0. the window object
// 1. the text to render, as a string
static int spn_SDL_Window_textSize(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, string); // the text to render

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	const char *text = STRARG(1);

	int w, h;
	TTF_SizeUTF8(window->font, text, &w, &h);

	*ret = spn_makehashmap();
	SpnHashMap *size = spn_hashmapvalue(ret);

	SpnValue width = spn_makeint(w);
	SpnValue height = spn_makeint(h);

	spn_hashmap_set_strkey(size, "width", &width);
	spn_hashmap_set_strkey(size, "height", &height);

	return 0;
}

//
// Library initialization and deinitialization
//

// the library has been loaded this many times
static unsigned init_refcount = 0;

// helper for building the hashmap representing the library
static void spn_SDL_construct_library(void)
{
	assert(library == NULL);
	library = spn_hashmap_new();

	// top-level library functions
	static const SpnExtFunc fns[] = {
		{ "OpenWindow",  spn_SDL_OpenWindow   },
		{ "PollEvent",   spn_SDL_PollEvent    },
		{ "StartTimer",  spn_SDL_StartTimer   },
		{ "StopTimer",   spn_SDL_StopTimer    }
	};

	for (size_t i = 0; i < sizeof fns / sizeof fns[0]; i++) {
		SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
		spn_hashmap_set_strkey(library, fns[i].name, &fnval);
		spn_value_release(&fnval);
	}

	// The Window class
	SpnHashMap *window = spn_hashmap_new();

	static const SpnExtFunc window_methods[] = {
		{ "refresh",           spn_SDL_Window_refresh           },
		{ "setColor",          spn_SDL_Window_setColor          },
		{ "getColor",          spn_SDL_Window_getColor          },
		{ "setFont",           spn_SDL_Window_setFont           },
		{ "clear",             spn_SDL_Window_clear             },
		{ "strokeRect",        spn_SDL_Window_strokeRect        },
		{ "fillRect",          spn_SDL_Window_fillRect          },
		{ "strokeArc",         spn_SDL_Window_strokeArc         },
		{ "fillArc",           spn_SDL_Window_fillArc           },
		{ "strokeEllipse",     spn_SDL_Window_strokeEllipse     },
		{ "fillEllipse",       spn_SDL_Window_fillEllipse       },
		{ "fillPolygon",       spn_SDL_Window_fillPolygon       },
		{ "strokeRoundedRect", spn_SDL_Window_strokeRoundedRect },
		{ "fillRoundedRect",   spn_SDL_Window_fillRoundedRect   },
		{ "bezier",            spn_SDL_Window_bezier            },
		{ "line",              spn_SDL_Window_line              },
		{ "point",             spn_SDL_Window_point             },
		{ "renderText",        spn_SDL_Window_renderText        },
		{ "textSize",          spn_SDL_Window_textSize          }
	};

	for (size_t i = 0; i < sizeof window_methods / sizeof window_methods[0]; i++) {
		SpnValue fnval = spn_makenativefunc(window_methods[i].name, window_methods[i].fn);
		spn_hashmap_set_strkey(window, window_methods[i].name, &fnval);
		spn_value_release(&fnval);
	}

	spn_hashmap_set_strkey(
		library,
		"Window",
		&(SpnValue){ .type = SPN_TYPE_HASHMAP, .v.o = window }
	);
	spn_object_release(window);
}

// when the last reference is gone to our library, we free the resources
static void spn_SDL_destroy_library(void)
{
	assert(library);
	spn_object_release(library);
	library = NULL;
}


// Library constructor and destructor
SPN_LIB_OPEN_FUNC(ctx) {
	if (init_refcount == 0) {
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			fprintf(stderr, "can't initialize SDL: %s\n", SDL_GetError());
			return spn_nilval;
		}

		spn_SDL_construct_library();
	}

	init_refcount++;

	spn_object_retain(library);
	return (SpnValue){ .type = SPN_TYPE_HASHMAP, .v.o = library };
}

SPN_LIB_CLOSE_FUNC(ctx) {
	if (--init_refcount == 0) {
		// free hashmap representing library
		spn_SDL_destroy_library();

		// deinitialize SDL
		SDL_Quit();
	}
}
