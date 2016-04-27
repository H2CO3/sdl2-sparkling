//
// sdl2_window.c
// sdl2-sparkling
//
// Created by Antonio Sarmento
// on 02/04/2016
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_window.h"
#include "sdl2_sparkling.h"
#include "helpers.h"
#include "sdl2_ttf.h"
#include "sdl2_event.h"
#include "sdl2_timer.h"
#include "sdl2_texture.h"
#include "sdl2_image.h"
#include "sdl2_gradient.h"

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
static SpnValue spn_SDL_Window_new(const char *title, int *width, int *height, Uint32 *ID)
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

	obj->font = NULL;

	*ID = SDL_GetWindowID(obj->window);
	return spn_makestrguserinfo(obj);
}

// Constructor for window objects.
int spnlib_SDL_OpenWindow(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, string);

	// construct return value, a window+renderer object
	*ret = spn_makehashmap();
	SpnHashMap *hm = spn_hashmapvalue(ret);

	// set its prototype
	SpnValue proto = spn_get_lib_prototype("Window");
	spn_hashmap_set_strkey(hm, "super", &proto);

	// actually open window
	int width = -1, height = -1;
	if (argc >= 3 && spn_isnumber(&argv[1]) && spn_isnumber(&argv[2])) {
		width = NUMARG(1);
		height = NUMARG(2);
	}

	Uint32 ID;
	SpnValue window = spn_SDL_Window_new(STRARG(0), &width, &height, &ID);

	// set its properties
	spn_hashmap_set_strkey(hm, "window", &window);
	spn_hashmap_set_strkey(hm, "width",  &(SpnValue){ .type = SPN_TYPE_INT, .v.i = width  });
	spn_hashmap_set_strkey(hm, "height", &(SpnValue){ .type = SPN_TYPE_INT, .v.i = height });
	spn_hashmap_set_strkey(hm, "ID",     &(SpnValue){ .type = SPN_TYPE_INT, .v.i = ID     });

	// handle ownership
	spn_value_release(&window);

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

	if (!spn_object_member_of_class(window, &spn_SDL_Window_class)) {
		return NULL;
	}

	return window;
}

/////////////////////////////////
//     Graphics primitives     //
/////////////////////////////////

// Dump ye ole video buffer!
static int spnlib_SDL_Window_refresh(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
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
static int spnlib_SDL_Window_clear(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
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

// Returns the SDL_BlendMode corresponding to the given string
// (Failsafe) Returns NONE if it doesn't correspond to any
static SDL_BlendMode get_blend_mode_value(const char *name)
{
	static const struct {
		const char *name;
		SDL_BlendMode mode;
	} modes[] = {
		{ "blend", SDL_BLENDMODE_BLEND },
		{ "add",   SDL_BLENDMODE_ADD   },
		{ "mod",   SDL_BLENDMODE_MOD   },
		{ "none",  SDL_BLENDMODE_NONE  }
	};

	for (size_t i = 0; i < COUNT(modes); i++) {
		if (strcmp(modes[i].name, name) == 0) {
			return modes[i].mode;
		}
	}

	// default to none
	return SDL_BLENDMODE_NONE;
}

// Does the inverse of the above function:
// returns a string corresponding to the given SDL_BlendMode
static const char *get_blend_mode_name(SDL_BlendMode mode)
{
	switch (mode) {
	case SDL_BLENDMODE_NONE:  return "none";
	case SDL_BLENDMODE_BLEND: return "blend";
	case SDL_BLENDMODE_ADD:   return "add";
	case SDL_BLENDMODE_MOD:   return "mod";
	default:                  return NULL;
	}
}

// Set the alpha blend mode
// Value is a string corresponding to one of SDL's 4 blend modes
// "blend", "add", "mod" or "none" (or anything else)
static int spnlib_SDL_Window_setBlendMode(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, string);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	const char *name = STRARG(1);
	SDL_BlendMode mode = get_blend_mode_value(name);

	SDL_SetRenderDrawBlendMode(renderer, mode);

	return 0;
}

// Returns string with name of the blend mode
static int spnlib_SDL_Window_getBlendMode(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;

	SDL_BlendMode mode;
	SDL_GetRenderDrawBlendMode(renderer, &mode);
	const char *name = get_blend_mode_name(mode);

	// we don't need to copy, the mode name is always
	// a string literal (statically allocated)
	*ret = spn_makestring_nocopy(name);

	return 0;
}


// Set the drawing color in RGBA format.
// Color components are expected to be floating-point values
// in the [0...1] closed interval.
static int spnlib_SDL_Window_setColor(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
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
static int spnlib_SDL_Window_getColor(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
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
// 3. font style string ("bold", "italic", "underline", "strikethrough", "normal"
//    or any space-spearated combination thereof.)
static int spnlib_SDL_Window_setFont(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
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
static int spnlib_SDL_Window_drawRect(
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

static int spnlib_SDL_Window_strokeRect(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_drawRect(ret, argc, argv, ctx, 0);
}

static int spnlib_SDL_Window_fillRect(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_drawRect(ret, argc, argv, ctx, 1);
}

// Draw arc with center (x, y), radius r
// 'start' and 'end' are the staring and ending angle of the
// outline of the arc, measured in radians.
// 'fill' means the same thing as above.
static int spnlib_SDL_Window_drawArc(
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
	// then gfx won't draw *anything* at all.
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

static int spnlib_SDL_Window_strokeArc(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_drawArc(ret, argc, argv, ctx, 0);
}

static int spnlib_SDL_Window_fillArc(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_drawArc(ret, argc, argv, ctx, 1);
}

// Draw an ellipse with center (x, y) and
// horizontal semi-axis rx, veritcal semi-axis ry
static int spnlib_SDL_Window_drawEllipse(
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

static int spnlib_SDL_Window_strokeEllipse(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_drawEllipse(ret, argc, argv, ctx, 0);
}

static int spnlib_SDL_Window_fillEllipse(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_drawEllipse(ret, argc, argv, ctx, 1);
}

// Fill the polygon enclosed by the points (x1, y1), (x2, y2), (x3, y3), ...
// At least 3 points must be specified.
static int spnlib_SDL_Window_fillPolygon(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, array);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;
	SpnArray *coords = ARRAYARG(1);

	size_t ncoords = spn_array_count(coords);
	size_t npoints = ncoords >> 1;

	if (ncoords % 2 != 0) {
		spn_ctx_runtime_error(ctx, "you must supply pairs of coordinates", NULL);
		return -2;
	}

	if (npoints < 3) {
		spn_ctx_runtime_error(ctx, "you must specify at least 3 points", NULL);
		return -3;
	}

	Sint16 vx[npoints];
	Sint16 vy[npoints];

	for (size_t i = 0; i < ncoords; i += 2) {
		SpnValue x = spn_array_get(coords, i);
		SpnValue y = spn_array_get(coords, i + 1);

		if (!spn_isnumber(&x) || !spn_isnumber(&y)) {
			spn_ctx_runtime_error(ctx, "coordinates must be numbers", NULL);
			return -4;
		}

		vx[i >> 1] = spn_intvalue_f(&x);
		vy[i >> 1] = spn_intvalue_f(&y);
	}

	Uint8 R, G, B, A;
	SDL_GetRenderDrawColor(renderer, &R, &G, &B, &A);

	filledPolygonRGBA(renderer, vx, vy, npoints, R, G, B, A);

	return 0;
}

// Stroke or fill rounded rectangle at point (x, y) of size (w, h)
// with corner radius r
static int spnlib_SDL_Window_drawRoundedRect(
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

static int spnlib_SDL_Window_strokeRoundedRect(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_drawRoundedRect(ret, argc, argv, ctx, 0);
}

static int spnlib_SDL_Window_fillRoundedRect(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_drawRoundedRect(ret, argc, argv, ctx, 1);
}

// Join in s steps the points (x1, y1), (x2, y2), (x3, y3), ...
// with a Bezier curve. 's', the number of steps determines how
// fine the resolution of the curve is (i. e., how close it is to a
// real curve - while it's just a line composed of straight segments)
static int spnlib_SDL_Window_bezier(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, int);
	CHECK_ARG_RETURN_ON_ERROR(2, array);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	SDL_Renderer *renderer = window->renderer;
	SpnArray *coords = ARRAYARG(2);

	int steps = INTARG(1);
	if (steps < 2) {
		spn_ctx_runtime_error(ctx, "you must specify at least 2 interpolation steps", NULL);
		return -2;
	}

	size_t ncoords = spn_array_count(coords);
	size_t npoints = ncoords >> 1; // divide by 2

	if (ncoords % 2 != 0) {
		spn_ctx_runtime_error(ctx, "you must supply pairs of coordinates", NULL);
		return -3;
	}

	if (npoints < 3) {
		spn_ctx_runtime_error(ctx, "you must specify at least 3 points", NULL);
		return -4;
	}

	Sint16 vx[npoints];
	Sint16 vy[npoints];

	for (size_t i = 0; i < ncoords; i += 2) {
		SpnValue x = spn_array_get(coords, i);
		SpnValue y = spn_array_get(coords, i + 1);

		if (!spn_isnumber(&x) || !spn_isnumber(&y)) {
			spn_ctx_runtime_error(ctx, "coordinates must be numbers", NULL);
			return -5;
		}

		vx[i >> 1] = spn_intvalue_f(&x);
		vy[i >> 1] = spn_intvalue_f(&y);
	}

	Uint8 R, G, B, A;
	SDL_GetRenderDrawColor(renderer, &R, &G, &B, &A);

	bezierRGBA(renderer, vx, vy, npoints, steps, R, G, B, A);

	return 0;
}

// Draw a straight 1px line between points (x, y) and (x + dx, y + dy)
// using the current drawing color.
static int spnlib_SDL_Window_line(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
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
static int spnlib_SDL_Window_point(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
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

// Draw 'text' with the current font,
// return a texture containing the result.
static int spnlib_SDL_Window_renderText(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, string); // text
	CHECK_ARG_RETURN_ON_ERROR(2, bool);   // rendering is high-quality?

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	if (window->font == NULL) {
		spn_ctx_runtime_error(ctx, "no font set; call setFont() first", NULL);
		return -2;
	}

	SDL_Renderer *renderer = window->renderer;

	const char *text = STRARG(1);
	bool hq = BOOLARG(2);

	spn_SDL_Texture *texture = spnlib_sdl2_render_text(
		renderer,
		text,
		window->font,
		hq
	);

	*ret = spn_makestrguserinfo(texture);
	return 0;
}

// parameters:
// 0. the window object
// 1. the text to render, as a string
static int spnlib_SDL_Window_textSize(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, string); // the text to render

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	if (window->font == NULL) {
		spn_ctx_runtime_error(ctx, "no font set; call setFont() first", NULL);
		return -2;
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

// Render texture in the given window
// Parameters:
// 0. the window object
// 1. the texture to render
// 2. X coordinate of the point to render at
// 3. Y coordinate of the point to render at
static int spnlib_SDL_Window_renderTexture(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, strguserinfo);
	CHECK_ARG_RETURN_ON_ERROR(2, number);
	CHECK_ARG_RETURN_ON_ERROR(3, number);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	spn_SDL_Texture *texture = OBJARG(1);
	if (!spn_object_member_of_class(texture, &spn_SDL_Texture_class)) {
		spn_ctx_runtime_error(ctx, "2nd argument is not a valid texture", NULL);
		return -2;
	}

	int x = NUMARG(2);
	int y = NUMARG(3);

	int w, h;
	SDL_QueryTexture(texture->texture, NULL, NULL, &w, &h);

	SDL_RenderCopy(
		window->renderer,
		texture->texture,
		NULL,
		&(SDL_Rect){ x, y, w, h }
	);

	return 0;
}

// Parses an image file and loads it into a texture object.
// Parameters:
// 0. the window object
// 1. the filename as a string
static int spnlib_SDL_Window_loadImage(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, string);

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	const char *filename = STRARG(1);
	spn_SDL_Texture *texture = spnlib_sdl2_load_image(window->renderer, filename);

	// return the loaded image if loading succeeded.
	// otherwise, implicitly return nil.
	if (texture) {
		*ret = spn_makestrguserinfo(texture);
	}

	return 0;
}

/////////////////////////////////
//////      GRADIENTS      //////
/////////////////////////////////

static int spnlib_SDL_Window_linearGradient(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap); // window
	CHECK_ARG_RETURN_ON_ERROR(1, number);  // w
	CHECK_ARG_RETURN_ON_ERROR(2, number);  // h
	CHECK_ARG_RETURN_ON_ERROR(3, number);  // delta x (for computing slope)
	CHECK_ARG_RETURN_ON_ERROR(4, number);  // delta y        - " -
	CHECK_ARG_RETURN_ON_ERROR(5, array);   // color-stops

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	int w = NUMARG(1);
	int h = NUMARG(2);
	double dx = NUMARG(3);
	double dy = NUMARG(4);

	SpnArray *arr = ARRAYARG(5);
	size_t n_stops = spn_array_count(arr);
	SPN_SDL_ColorStop color_stops[n_stops];
	bool success = spnlib_sdl2_array_to_colorstop(arr, color_stops);

	if (!success) {
		spn_ctx_runtime_error(ctx, "invalid color stop specification", NULL);
		return -2;
	}

	SDL_Texture *texture = spnlib_sdl2_linear_gradient(
		window->renderer,
		w,
		h,
		dx,
		dy,
		color_stops,
		n_stops
	);

	if (texture == NULL) {
		spn_ctx_runtime_error(ctx, "invalid dimensions or bad number of color stops", NULL);
		return -3;
	}

	spn_SDL_Texture *obj = spnlib_SDL_texture_new(texture);
	*ret = spn_makestrguserinfo(obj);
	return 0;
}

static int spnlib_SDL_Window_ellipsoidalGradient(
	SpnValue *ret,
	int argc,
	SpnValue *argv,
	void *ctx,
	SDL_Texture *(*gradientPainter)(
		SDL_Renderer *renderer,
		int rx,
		int ry,
		const SPN_SDL_ColorStop color_stops[],
		unsigned n
	)
)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap); // window
	CHECK_ARG_RETURN_ON_ERROR(1, number);  // rx
	CHECK_ARG_RETURN_ON_ERROR(2, number);  // ry
	CHECK_ARG_RETURN_ON_ERROR(3, array);   // color-stops

	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	int rx = NUMARG(1);
	int ry = NUMARG(2);

	SpnArray *arr = ARRAYARG(3);
	size_t n_stops = spn_array_count(arr);
	SPN_SDL_ColorStop color_stops[n_stops];
	bool success = spnlib_sdl2_array_to_colorstop(arr, color_stops);

	if (!success) {
		spn_ctx_runtime_error(ctx, "invalid color stop specification", NULL);
		return -2;
	}

	SDL_Texture *texture = gradientPainter(
		window->renderer,
		rx,
		ry,
		color_stops,
		n_stops
	);

	if (texture == NULL) {
		spn_ctx_runtime_error(ctx, "invalid dimensions or bad number of color stops", NULL);
		return -3;
	}

	spn_SDL_Texture *obj = spnlib_SDL_texture_new(texture);
	*ret = spn_makestrguserinfo(obj);
	return 0;
}

static int spnlib_SDL_Window_radialGradient(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_ellipsoidalGradient(
		ret,
		argc,
		argv,
		ctx,
		spnlib_sdl2_radial_gradient
	);
}

static int spnlib_SDL_Window_conicalGradient(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	return spnlib_SDL_Window_ellipsoidalGradient(
		ret,
		argc,
		argv,
		ctx,
		spnlib_sdl2_conical_gradient
	);
}

/////////////////////////////////
///////    Message Box    ///////
/////////////////////////////////

static Uint32 get_messagebox_flag(const char *str) {
	static const struct {
		const char *name;
		Uint32 flag;
	} flags[] = {
		{ "error",       SDL_MESSAGEBOX_ERROR       },
		{ "warning",     SDL_MESSAGEBOX_WARNING     },
		{ "information", SDL_MESSAGEBOX_INFORMATION },
		{ "info",        SDL_MESSAGEBOX_INFORMATION }
	};

	for (size_t i = 0; i < sizeof flags / sizeof flags[0]; i++) {
		if (strcmp(flags[i].name, str) == 0) {
			return flags[i].flag;
		}
	}
	// Defaults to error
	return SDL_MESSAGEBOX_ERROR;
}

static Uint32 get_messagebox_button_flag(const char *str) {
	static const struct {
		const char *name;
		Uint32 flag;
	} flags[] = {
		{ "return", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT },
		{ "escape", SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT }
	};

	for (size_t i = 0; i < COUNT(flags); i++) {
		if (strcmp(flags[i].name, str) == 0) {
			return flags[i].flag;
		}
	}
	// Defaults to 0
	return 0;
}

static SDL_MessageBoxButtonData *get_messagebox_button_data
(
	SpnHashMap *hm,
	int count
)
{
	SpnValue name, flag;
	size_t cursor = 0, i = 0;
	SDL_MessageBoxButtonData *buttondata = SDL_malloc(count * sizeof(SDL_MessageBoxButtonData));

	while ((cursor = spn_hashmap_next(hm, cursor, &name, &flag)) != 0) {
		buttondata[i].flags = get_messagebox_button_flag(spn_stringvalue(&flag)->cstr);
		buttondata[i].buttonid = i;
		buttondata[i].text = spn_stringvalue(&name)->cstr;
		i++;
	}

	return buttondata;
}

static SDL_MessageBoxData get_message_box_data
(
	Uint32 flags,
	SDL_Window *window,
	const char *title,
	const char *message,
	SpnHashMap *buttons
)
{
	SDL_MessageBoxData data;

	data.flags = flags;
	data.window = window;
	data.title = title;
	data.message = message;
	data.numbuttons = spn_hashmap_count(buttons);
	data.buttons = get_messagebox_button_data(buttons, data.numbuttons);
	data.colorScheme = NULL;

	return data;
}

static int spnlib_SDL_Window_ShowMessageBox(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, hashmap);
	CHECK_ARG_RETURN_ON_ERROR(1, string);
	CHECK_ARG_RETURN_ON_ERROR(2, string);
	CHECK_ARG_RETURN_ON_ERROR(3, string);
	if (argc > 4) {
		CHECK_ARG_RETURN_ON_ERROR(4, hashmap);
	}

	SDL_MessageBoxData data;
	int buttonid;
	SpnHashMap *hm = HASHMAPARG(0);
	spn_SDL_Window *window = window_from_hashmap(hm);

	if (window == NULL) {
		spn_ctx_runtime_error(ctx, "window object is invalid", NULL);
		return -1;
	}

	Uint32 flags = get_messagebox_flag(STRARG(1));
	const char *title = STRARG(2);
	const char *msg = STRARG(3);

	if (argc == 4) {
		SDL_ShowSimpleMessageBox(flags, title, msg, window->window);
	} else {
		data = get_message_box_data(flags, window->window, title, msg, HASHMAPARG(4));
		SDL_ShowMessageBox(&data, &buttonid);

		SDL_free(data.buttons);
		*ret = spn_makeint(buttonid);
	}

	return 0;
}

//
// Window methods hashmap creation
//

void spnlib_SDL_methods_for_Window(SpnHashMap *window)
{
	static const SpnExtFunc methods[] = {
		{ "refresh",           spnlib_SDL_Window_refresh           },
		{ "setBlendMode",      spnlib_SDL_Window_setBlendMode      },
		{ "getBlendMode",      spnlib_SDL_Window_getBlendMode      },
		{ "setColor",          spnlib_SDL_Window_setColor          },
		{ "getColor",          spnlib_SDL_Window_getColor          },
		{ "setFont",           spnlib_SDL_Window_setFont           },
		{ "clear",             spnlib_SDL_Window_clear             },
		{ "strokeRect",        spnlib_SDL_Window_strokeRect        },
		{ "fillRect",          spnlib_SDL_Window_fillRect          },
		{ "strokeArc",         spnlib_SDL_Window_strokeArc         },
		{ "fillArc",           spnlib_SDL_Window_fillArc           },
		{ "strokeEllipse",     spnlib_SDL_Window_strokeEllipse     },
		{ "fillEllipse",       spnlib_SDL_Window_fillEllipse       },
		{ "fillPolygon",       spnlib_SDL_Window_fillPolygon       },
		{ "strokeRoundedRect", spnlib_SDL_Window_strokeRoundedRect },
		{ "fillRoundedRect",   spnlib_SDL_Window_fillRoundedRect   },
		{ "bezier",            spnlib_SDL_Window_bezier            },
		{ "line",              spnlib_SDL_Window_line              },
		{ "point",             spnlib_SDL_Window_point             },
		{ "renderText",        spnlib_SDL_Window_renderText        },
		{ "textSize",          spnlib_SDL_Window_textSize          },
		{ "renderTexture",     spnlib_SDL_Window_renderTexture     },
		{ "loadImage",         spnlib_SDL_Window_loadImage         },
		{ "linearGradient",    spnlib_SDL_Window_linearGradient    },
		{ "radialGradient",    spnlib_SDL_Window_radialGradient    },
		{ "conicalGradient",   spnlib_SDL_Window_conicalGradient   },
		{ "showMessageBox",    spnlib_SDL_Window_ShowMessageBox    }
	};

	for (size_t i = 0; i < COUNT(methods); i++) {
		SpnValue fnval = spn_makenativefunc(methods[i].name, methods[i].fn);
		spn_hashmap_set_strkey(window, methods[i].name, &fnval);
		spn_value_release(&fnval);
	}
}
