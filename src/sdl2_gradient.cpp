//
// sdl2_gradient.cpp
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 05/04/2015
//
// Licensed under the 2-clause BSD License
//


#include "sdl2_gradient.h"
#include <SDL2/SDL2_gfxPrimitives.h>

#include <spn/hashmap.h>

#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cmath>

#define RMASK 0xff000000
#define GMASK 0x00ff0000
#define BMASK 0x0000ff00
#define AMASK 0x000000ff

#define RGBA32(rv, gv, bv, av) \
	((Uint32(rv) << 24) | (Uint32(gv) << 16) | (Uint32(bv) << 8) | (Uint32(av) << 0))


// used for saving and restoring the current
// drawing color of a renderer
class RenderColorGuard {
	SDL_Renderer *renderer;
	Uint8 r, g, b, a;

public:
	RenderColorGuard(SDL_Renderer *rend) : renderer(rend)
	{
		SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	}

	~RenderColorGuard()
	{
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
	}
};


bool spnlib_sdl2_array_to_colorstop(
	SpnArray *arr,
	SPN_SDL_ColorStop color_stops[]
)
{
	std::size_t n_stops = spn_array_count(arr);

	for (std::size_t i = 0; i < n_stops; i++) {
		SpnValue cstp_val = spn_array_get(arr, i);
		if (!spn_ishashmap(&cstp_val)) {
			return false;
		}

		// each element in the array is a hashmap representing
		// a color stop point. The following keys must be present:
		// r, g, b, a (doubles between 0...1) are the color components
		// p is also a double in [0...1] encoding the progress
		// (position, ratio, etc.) of the color stop point.
		SpnHashMap *cstp = spn_hashmapvalue(&cstp_val);

		SpnValue r = spn_hashmap_get_strkey(cstp, "r");
		SpnValue g = spn_hashmap_get_strkey(cstp, "g");
		SpnValue b = spn_hashmap_get_strkey(cstp, "b");
		SpnValue a = spn_hashmap_get_strkey(cstp, "a");
		SpnValue p = spn_hashmap_get_strkey(cstp, "p");

		if (!spn_isnumber(&r) || !spn_isnumber(&g) || !spn_isnumber(&b)
		 || !spn_isnumber(&a) || !spn_isnumber(&p)) {
			return false;
		}

		auto constrain_to_01 = [](double x) { return std::max(0.0, std::min(x, 1.0)); };

		color_stops[i].progress = constrain_to_01(spn_floatvalue_f(&p));
		color_stops[i].color = {
			/* .r = */ Uint8(255 * constrain_to_01(spn_floatvalue_f(&r))),
			/* .g = */ Uint8(255 * constrain_to_01(spn_floatvalue_f(&g))),
			/* .b = */ Uint8(255 * constrain_to_01(spn_floatvalue_f(&b))),
			/* .a = */ Uint8(255 * constrain_to_01(spn_floatvalue_f(&a)))
		};
	}

	// sort color stops by progress, since
	// it is required by std::lower_bound
	std::sort(
		color_stops,
		color_stops + n_stops,
		[](SPN_SDL_ColorStop cs1, SPN_SDL_ColorStop cs2) {
			return cs1.progress < cs2.progress;
		}
	);

	return true;
}

static Uint32 interpolate_color_rgba32(
	const SPN_SDL_ColorStop color_stops[],
	unsigned n,
	double p
)
{
	// find which two color stop points 'p' is in between,
	// i. e. for which 'idx' holds 'p <= stop_points[idx]'
	auto it = std::lower_bound(
		color_stops,
		color_stops + n,
		p,
		[](SPN_SDL_ColorStop cs1, double ps) {
			return cs1.progress < ps;
		}
	);

	// first, check degenerate cases
	if (it == color_stops) {
		// degenerate case: first color-stop point is
		// already >= p (probably p is 0 or negative)
		auto c = color_stops[0].color;
		return RGBA32(c.r, c.g, c.b, c.a);
	} else if (it == color_stops + n) {
		// degenerate case: p > maximal color-stop point
		// (there's no higher color stop point to interpolate
		// towards, p is probably 1 or more)
		auto c = color_stops[n - 1].color;
		return RGBA32(c.r, c.g, c.b, c.a);
	}

	// non-degenerate case: p falls somewhere between
	// two distinct neighboring color-stop points.
	// Interpolate between these points to find the
	// appropriate linear combination of colors.
	double p0 = it[-1].progress;
	double p1 = it[0].progress;
	double q = (p - p0) / (p1 - p0);

	// return (1 - q) * it[-1].color + q * it[0].color;
	// https://www.youtube.com/watch?v=LKnqECcg6Gw
	auto c0 = it[-1].color;
	auto c1 = it[0].color;

	Uint8 r = std::sqrt(c0.r * c0.r * (1 - q) + c1.r * c1.r * q);
	Uint8 g = std::sqrt(c0.g * c0.g * (1 - q) + c1.g * c1.g * q);
	Uint8 b = std::sqrt(c0.b * c0.b * (1 - q) + c1.b * c1.b * q);
	Uint8 a = std::sqrt(c0.a * c0.a * (1 - q) + c1.a * c1.a * q);

	return RGBA32(r, g, b, a);
}

static SDL_Texture *renderPixelBuffer(
	SDL_Renderer *renderer,
	std::vector<Uint32> &buf, // must be non-const, blame SDL_CreateRGBSurfaceFrom
	int width,
	int height
)
{
	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
		buf.data(),
		width,
		height,
		32,
		width * sizeof buf[0],
		RMASK,
		GMASK,
		BMASK,
		AMASK
	);

	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);

	return texture;
}


SDL_Texture *spnlib_sdl2_linear_gradient(
	SDL_Renderer *renderer,
	int w,
	int h,
	double vx,
	double vy,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n
)
{
	// negative width/height make no sense
	if (w < 0 || h < 0) {
		return NULL;
	}

	// direction vector cannot be null vector
	if (vx == 0 && vy == 0) {
		return NULL;
	}

	// at least 2 color stop points (start, end) are required
	if (n < 2) {
		return NULL;
	}

	// Save original drawing color
	RenderColorGuard cg(renderer);

	// Prepare pixel buffer
	std::vector<Uint32> buf;
	buf.reserve(w * h);

	// compute length of pivot color line clipped to bounds.
	// treat infinities correctly, avoid division by zero.
	bool pivot_is_steep = std::abs(vy * w) > std::abs(vx * h);
	double pivot_length;
	if (pivot_is_steep) {
		// vx may be 0
		pivot_length = h * std::sqrt(1 + (vx * vx) / (vy * vy));
	} else {
		// vy may be 0
		pivot_length = w * std::sqrt(1 + (vy * vy) / (vx * vx));
	}

	double norm = std::sqrt(vx * vx + vy * vy);
	double c_coeff = (vx * w + vy * h) / 2;

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			// progress along the pivot color line is the ratio of
			// the _signed_ distance of the point (x, y) from the
			// perpendicular bisector of the pivot color line and
			// the length of the pivot line. This falls into the
			// range [-0.5, +0.5], so we add 0.5 in to normalize
			// it to the range [0...1]
			double dist = (vx * x + vy * y - c_coeff) / norm;

			// compute color stop parameter
			double p = 0.5 + dist / pivot_length;

			// interpolate between neighboring color stops
			Uint32 c = interpolate_color_rgba32(color_stops, n, p);
			buf.push_back(c);
		}
	}

	// render prepared pixel array
	return renderPixelBuffer(
		renderer,
		buf,
		w,
		h
	);
}

static SDL_Texture *spnlib_sdl2_ellipsoidal_gradient(
	SDL_Renderer *renderer,
	int rx,
	int ry,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n,
	bool isRadial
)
{
	// negative radii make no sense
	if (rx < 0 || ry < 0) {
		return NULL;
	}

	// at least two color stop points must be specified (begin, end)
	if (n < 2) {
		return NULL;
	}

	// Save original drawing color
	RenderColorGuard cg(renderer);

	// prepare buffer with transparent pixels
	std::vector<Uint32> buf(2 * rx * 2 * ry, RGBA32(0, 0, 0, 0));

	for (int y = -ry; y < +ry; y++) {
		for (int x = -rx; x < +rx; x++) {
			// check if point is within ellipse
			// r = normalized "radius"
			double r = std::sqrt(double(x * x) / (rx * rx) + double(y * y) / (ry * ry));
			if (r > 1) {
				continue;
			}

			// for a radial gradient, the color-stop progress is the normalized radius.
			// for a conical one, it is the normalized (divided-by-two-pi) direction angle.
			const double pi = 4 * std::atan(1), tau = 2 * pi;
			double p = isRadial ? r : (std::atan2(y, x) + pi) / tau;

			// interpolate between colors
			Uint32 c = interpolate_color_rgba32(color_stops, n, p);
			buf[2 * rx * (ry + y) + rx + x] = c;
		}
	}

	// blit pixels at once
	return renderPixelBuffer(
		renderer,
		buf,
		2 * rx,
		2 * ry
	);
}

SDL_Texture *spnlib_sdl2_radial_gradient(
	SDL_Renderer *renderer,
	int rx,
	int ry,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n
)
{
	return spnlib_sdl2_ellipsoidal_gradient(
		renderer,
		rx,
		ry,
		color_stops,
		n,
		true
	);
}

SDL_Texture *spnlib_sdl2_conical_gradient(
	SDL_Renderer *renderer,
	int rx,
	int ry,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n
)
{
	return spnlib_sdl2_ellipsoidal_gradient(
		renderer,
		rx,
		ry,
		color_stops,
		n,
		false
	);
}
