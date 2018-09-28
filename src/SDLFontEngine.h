/*
Copyright © 2011-2012 Clint Bellanger and Thane Brimhall
Copyright © 2014 Henrik Andersson
Copyright © 2015-2016 Justin Jacobs

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

#ifndef SDL_FONT_ENGINE_H
#define SDL_FONT_ENGINE_H

#include "FontEngine.h"
#include <SDL_ttf.h>

class SDLFontStyle : public FontStyle {
public:
	SDLFontStyle();
	~SDLFontStyle() {};

	TTF_Font *ttfont;
};

/**
 *
 * class SDLFontEngine
 * Handles rendering a bitmap font using SDL TTF_Font.
 *
 */

class SDLFontEngine : public FontEngine {
private:
	std::vector<SDLFontStyle> font_styles;
	SDLFontStyle *active_font;

protected:
	void renderInternal(const std::string& text, int x, int y, int justify, Image *target, const Color& color);

public:
	SDLFontEngine();
	~SDLFontEngine();

	int getLineHeight();
	int getFontHeight();

	void setFont(const std::string& _font);

	int calc_width(const std::string& text);
	std::string trimTextToWidth(const std::string& text, const int width, const bool use_ellipsis, size_t left_pos);
};

#endif
