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


bool spnlib_sdl2_array_to_colorstop(
	SpnArray *arr,
	SPN_SDL_ColorStop color_stops[]
)
{
	std::size_t n_stops = spn_array_count(arr);

	for (size_t i = 0; i < n_stops; i++) {
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

		color_stops[i].p = constrain_to_01(spn_floatvalue_f(&p));
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
			return cs1.p < cs2.p;
		}
	);

	return true;
}

static SDL_Color interpolate_color(
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
			return cs1.p < ps;
		}
	);

	// first, check degenerate cases
	if (it == color_stops) {
		// degenerate case: first color-stop point is
		// already >= p (probably p is 0 or negative)
		return color_stops[0].color;
	} else if (it == color_stops + n) {
		// degenerate case: p > maximal color-stop point
		// (there's no higher color stop point to interpolate
		// towards, p is probably 1 or more)
		return color_stops[n - 1].color;
	}

	// non-degenerate case: p falls somewhere between
	// two distinct neighboring color-stop points.
	// Interpolate between these points to find the
	// appropriate linear combination of colors.
	double p0 = it[-1].p;
	double p1 = it[0].p;
	double q = (p - p0) / (p1 - p0);

	// return (1 - q) * it[-1].color + q * it[0].color;
	// https://www.youtube.com/watch?v=LKnqECcg6Gw
	auto c0 = it[-1].color;
	auto c1 = it[0].color;
	Uint8 r = std::sqrt(c0.r * c0.r * (1 - q) + c1.r * c1.r * q);
	Uint8 g = std::sqrt(c0.g * c0.g * (1 - q) + c1.g * c1.g * q);
	Uint8 b = std::sqrt(c0.b * c0.b * (1 - q) + c1.b * c1.b * q);
	Uint8 a = std::sqrt(c0.a * c0.a * (1 - q) + c1.a * c1.a * q);
	return { r, g, b, a };
}

static void renderPixelBuffer(
	SDL_Renderer *renderer,
	std::vector<Uint32> &buf, // must be non-const, blame SDL_CreateRGBSurfaceFrom
	int x,
	int y,
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
	SDL_Rect rect { x, y, width, height };

	SDL_RenderCopy(renderer, texture, NULL, &rect);
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(surface);
}


void spnlib_sdl2_linear_gradient(
	SDL_Renderer *renderer,
	SDL_Point start,
	SDL_Point end,
	double m, // slope of gradient's main color line
	const SPN_SDL_ColorStop color_stops[],
	unsigned n // number of color_stops
)
{
	// Save original drawing color
	Uint8 ro, go, bo, ao;
	SDL_GetRenderDrawColor(renderer, &ro, &go, &bo, &ao);

	int dx = end.x - start.x;
	int dy = end.y - start.y;

	// Prepare pixel buffer
	std::vector<Uint32> buf;
	buf.reserve(std::abs(dx * dy));

	for (int y = 0; y < std::abs(dy); y++) {
		for (int x = 0; x < std::abs(dx); x++) {
			// Project point to the 'main' color line crossing the origin.
			// On this line, the progress 'p' is proportional
			// to the distance from the origin (1 = the lenght of the diagonal).
			// 'isochromatic' lines are perpendicular to this line: m_perp = -1 / m
			int delta_x, delta_y;
			if (m == 0) {
				delta_x = 0;
				delta_y = -y;
			} else if (1 / m == 0) {
				// +/- infty
				delta_x = -x;
				delta_y = 0;
			} else {
				delta_x = (y - m * x) / (m + 1 / m);
				delta_y = -1 / m * delta_x;
			}

			int xp = x + delta_x;
			int yp = y + delta_y;

			// compute color stop parameter
			double p;
			if (std::abs(m) < std::abs(double(dy) / dx)) {
				p = std::abs(double(xp) / dx);
			} else {
				p = std::abs(double(yp) / dy);
			}

			// interpolate between neighboring color stops
			auto c = interpolate_color(color_stops, n, p);
			buf.push_back(RGBA32(c.r, c.g, c.b, c.a));
		}
	}

	// render prepared pixel array
	renderPixelBuffer(
		renderer,
		buf,
		std::min(start.x, end.x),
		std::min(start.y, end.y),
		std::abs(dx),
		std::abs(dy)
	);

	// Restore original drawing color
	SDL_SetRenderDrawColor(renderer, ro, go, bo, ao);
}

static void spnlib_sdl2_ellipsoidal_gradient(
	SDL_Renderer *renderer,
	SDL_Point center,
	int rx,
	int ry,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n,
	bool isRadial
)
{
	// Save original drawing color
	Uint8 ro, go, bo, ao;
	SDL_GetRenderDrawColor(renderer, &ro, &go, &bo, &ao);

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
			auto c = interpolate_color(color_stops, n, p);
			buf[2 * rx * (ry + y) + rx + x] = RGBA32(c.r, c.g, c.b, c.a);
		}
	}

	// blit pixels at once
	renderPixelBuffer(renderer, buf, center.x - rx, center.y - ry, 2 * rx, 2 * ry);

	// Restore original drawing color
	SDL_SetRenderDrawColor(renderer, ro, go, bo, ao);
}

void spnlib_sdl2_radial_gradient(
	SDL_Renderer *renderer,
	SDL_Point center,
	int rx,
	int ry,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n
)
{
	return spnlib_sdl2_ellipsoidal_gradient(
		renderer,
		center,
		rx,
		ry,
		color_stops,
		n,
		true
	);
}

void spnlib_sdl2_conical_gradient(
	SDL_Renderer *renderer,
	SDL_Point center,
	int rx,
	int ry,
	const SPN_SDL_ColorStop color_stops[],
	unsigned n
)
{
	return spnlib_sdl2_ellipsoidal_gradient(
		renderer,
		center,
		rx,
		ry,
		color_stops,
		n,
		false
	);
}
