/*
Copyright © 2011-2012 Clint Bellanger and Thane Brimhall
Copyright © 2014 Henrik Andersson

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

#ifndef FONT_ENGINE_H
#define FONT_ENGINE_H

#include "CommonIncludes.h"
#include "Utils.h"

#include <SDL_ttf.h>

const int JUSTIFY_LEFT = 0;
const int JUSTIFY_RIGHT = 1;
const int JUSTIFY_CENTER = 2;

const Color FONT_WHITE = Color(255,255,255);
const Color FONT_BLACK = Color(0,0,0);

class FontStyle {
public:
	std::string name;
	std::string path;
	int ptsize;
	bool blend;
	TTF_Font *ttfont;
	int line_height;
	int font_height;

	FontStyle();
};

/**
 * class FontEngine
 *
 * Handles rendering a bitmap font.
 */

class FontEngine {
private:
	std::map<std::string,Color> color_map;
	std::vector<FontStyle> font_styles;
	FontStyle *active_font;

public:
	FontEngine();
	~FontEngine();

	int getLineHeight() {
		return active_font->line_height;
	}
	int getFontHeight() {
		return active_font->font_height;
	}

	Color getColor(std::string _color);
	void setFont(std::string _font);

	int calc_width(const std::string& text);
	Point calc_size(const std::string& text_with_newlines, int width);

	void render(const std::string& text, int x, int y, int justify, Image *target, Color color);
	void render(const std::string& text, int x, int y, int justify, Image *target, int width, Color color);
	void renderShadowed(const std::string& text, int x, int y, int justify, Image *target, Color color);
	void renderShadowed(const std::string& text, int x, int y, int justify, Image *target, int width, Color color);

	int cursor_y;
};

#endif
