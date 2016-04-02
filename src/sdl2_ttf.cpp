//
// sdl2_ttf.cpp
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 30/03/2015
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_ttf.h"

#include <unordered_map>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <iterator>
#include <vector>
#include <array>
#include <cstring>


// Initializes and deinitializes TTF subsystem
// Since C++ guarantees that within a translation unit,
// static objects are initialized in the order of their
// definition, we declare/define this object first, so
// the cache of TTF_Font pointers is initialized *after*
// this object, and consequently, it is destroyed *before*
// this object. This is necessary because one must not
// even touch the library after calling 'TTF_Quit()' - and
// indeed, trying to call 'TTF_CloseFont()' after quitting
// results in a crash (for me, at least).
static const struct TTF_InitGuard {
	TTF_InitGuard() {
		TTF_Init();
	}
	~TTF_InitGuard() {
		TTF_Quit();
	}
} initGuard;

struct FontKey {
	const char *name;
	int ptsize;

	bool operator==(FontKey that) const {
		return std::strcmp(name, that.name) == 0 && ptsize == that.ptsize;
	}
};

namespace std {
	template<>
	struct hash<const char *> {
		size_t operator()(const char *str) {
			size_t h = 5381;
			for (const char *p = str; *p; p++) {
				h *= 33;
				h ^= static_cast<unsigned char>(*p);
			}
			return h;
		}
	};

	template<>
	struct hash<FontKey> {
		size_t operator()(FontKey key) const {
			return std::hash<const char *>()(key.name) ^ std::hash<int>()(key.ptsize);
		}
	};
}

#include <iostream>

struct FontDeleter {
	void operator()(TTF_Font *font) const {
		TTF_CloseFont(font);
	}
};

TTF_Font *spnlib_sdl2_get_font(
	const char *name,
	int ptsize,
	const char *style
)
{
	static std::unordered_map<
		FontKey,
		std::unique_ptr<
			TTF_Font,
			FontDeleter
		>
	> cache;

	auto &ptr = cache[{ name, ptsize }];

	if (ptr == nullptr) {
		std::string fname(name);
		fname += ".ttf";
		ptr = std::unique_ptr<TTF_Font, FontDeleter> {
			TTF_OpenFont(fname.c_str(), ptsize)
		};
	}

	// could not open font
	if (ptr == nullptr) {
		return nullptr;
	}

	TTF_Font *font = ptr.get();

	// Set style
	static std::unordered_map<std::string, int> styles {
		{ "bold",          TTF_STYLE_BOLD          },
		{ "italic",        TTF_STYLE_ITALIC        },
		{ "underline",     TTF_STYLE_UNDERLINE     },
		{ "strikethrough", TTF_STYLE_STRIKETHROUGH },
		{ "normal",        TTF_STYLE_NORMAL        }
	};

	if (style == nullptr) {
		style = "normal";
	}

	std::istringstream sss(style);
	std::vector<std::string> stylev {
		std::istream_iterator<std::string> { sss },
		std::istream_iterator<std::string> {}
	};

	int stylemask = TTF_STYLE_NORMAL;
	for (const auto &s : stylev) {
		stylemask |= styles[s];
	}

	TTF_SetFontStyle(font, stylemask);

	return font;
}

spn_SDL_Texture *spnlib_sdl2_render_text(
	SDL_Renderer *renderer,
	const char *text,
	TTF_Font *font,
	bool hq
)
{
	std::array<SDL_Surface *(*)(TTF_Font *, const char *, SDL_Color), 2> renderers {
		TTF_RenderUTF8_Solid,
		TTF_RenderUTF8_Blended
	};

	SDL_Color color;
	SDL_GetRenderDrawColor(renderer, &color.r, &color.g, &color.b, &color.a);

	// Render text to surface, convert to texture
	SDL_Surface *surface = renderers[hq](font, text, color);
	return spnlib_SDL_texture_new_surface(renderer, surface);
}
