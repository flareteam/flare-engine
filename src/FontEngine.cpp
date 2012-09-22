/*
Copyright © 2011-2012 Clint Bellanger and Thane Brimhall

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

/*
 * class FontEngine
 */

#include "SDL_gfxBlitFunc.h"
#include "FontEngine.h"
#include "FileParser.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsParsing.h"
#include <iostream>
#include <sstream>

using namespace std;


FontEngine::FontEngine() {
	font_pt = 10;
	render_blended = false;

	// Initiate SDL_ttf
	if(!TTF_WasInit() && TTF_Init()==-1) {
		printf("TTF_Init: %s\n", TTF_GetError());
		exit(2);
	}

	// load the font
	string font_path;
	FileParser infile;
	if (infile.open(mods->locate("engine/font_settings.txt"))) {
		while (infile.next()) {
			if (infile.key == "font_regular") {
				font_path = infile.val;
			}
			else if (infile.key == "ptsize") {
				font_pt = atoi(infile.val.c_str());
			}
			else if (infile.key == "render") {
				if (infile.val == "blended") render_blended = true;
			}
		}
		infile.close();
	} else fprintf(stderr, "Unable to open engine/font_settings.txt!\n");
	// Redefine font to language specific if avaliable
	std::string local_font;
	if (infile.open(mods->locate("engine/languages.txt"))) {
		while (infile.next()) {
			if (infile.key == LANGUAGE) {
			   local_font = infile.nextValue();// Ignore full language name
			   local_font = infile.nextValue();
			   if (local_font != "") {
				   font_path = local_font; font_pt = atoi(infile.nextValue().c_str());
			   }
			}
		}
		infile.close();
	} else fprintf(stderr, "Unable to open engine/languages.txt!\n");
	font_path = mods->locate("fonts/" + font_path);
	ttfont = TTF_OpenFont(font_path.c_str(), font_pt);
	if(!ttfont) printf("TTF_OpenFont: %s\n", TTF_GetError());

	// calculate the optimal line height
	line_height = TTF_FontLineSkip(ttfont);
	font_height = TTF_FontHeight(ttfont);

	// set the font colors
	// RGB values, the last value is 'unused'. For info,
	// see http://www.libsdl.org/cgi/docwiki.cgi/SDL_Color
	SDL_Color color;
	if (infile.open(mods->locate("engine/font_colors.txt"))) {
		while (infile.next()) {
			infile.val = infile.val + ',';
			color.r = eatFirstInt(infile.val,',');
			color.g = eatFirstInt(infile.val,',');
			color.b = eatFirstInt(infile.val,',');
			color_map[infile.key] = color;
		}
		infile.close();
	} else fprintf(stderr, "Unable to open engine/font_colors.txt!\n");
}

SDL_Color FontEngine::getColor(string _color) {
	map<string,SDL_Color>::iterator it,end;
	for (it=color_map.begin(), end=color_map.end(); it!=end; ++it) {
		if (_color.compare(it->first) == 0) return it->second;
	}
	// If all else fails, return white;
	return FONT_WHITE;
}

/**
 * For single-line text, just calculate the width
 */
int FontEngine::calc_width(const std::string& text) {
	int w, h;
	TTF_SizeUTF8(ttfont, text.c_str(), &w, &h);
	return w;
}

/**
 * Using the given wrap width, calculate the width and height necessary to display this text
 */
Point FontEngine::calc_size(const std::string& text_with_newlines, int width) {
	char newline = 10;

	string text = text_with_newlines;

	// if this contains newlines, recurse
	size_t check_newline = text.find_first_of(newline);
	if (check_newline != string::npos) {
		Point p1 = calc_size(text.substr(0, check_newline), width);
		Point p2 = calc_size(text.substr(check_newline+1, text.length()), width);
		Point p3;

		if (p1.x > p2.x) p3.x = p1.x;
		else p3.x = p2.x;

		p3.y = p1.y + p2.y;
		return p3;
	}

	int height = 0;
	int max_width = 0;

	string next_word;
	stringstream builder;
	stringstream builder_prev;
	char space = 32;
	size_t cursor = 0;
	string fulltext = text + " ";

	builder.str("");
	builder_prev.str("");

	next_word = getNextToken(fulltext, cursor, space);

	while(cursor != string::npos) {
		builder << next_word;

		if (calc_width(builder.str()) > width) {

			// this word can't fit on this line, so word wrap
			height = height + getLineHeight();
			if (calc_width(builder_prev.str()) > max_width) {
				max_width = calc_width(builder_prev.str());
			}

			builder_prev.str("");
			builder.str("");

			builder << next_word << " ";
		}
		else {
			builder <<  " ";
			builder_prev.str(builder.str());
		}

		next_word = getNextToken(fulltext, cursor, space); // get next word
	}

	height = height + getLineHeight();
	builder.str(trim(builder.str(), ' ')); //removes whitespace that shouldn't be included in the size
	if (calc_width(builder.str()) > max_width) max_width = calc_width(builder.str());

	Point size;
	size.x = max_width;
	size.y = height;
	return size;
}


/**
 * Render the given text at (x,y) on the target image.
 * Justify is left, right, or center
 */
void FontEngine::render(const std::string& text, int x, int y, int justify, SDL_Surface *target, SDL_Color color) {
	int dest_x = -1;
	int dest_y = -1;

	// DEBUG
	dest_x = x;
	dest_y = y;

	// calculate actual starting x,y based on justify
	if (justify == JUSTIFY_LEFT) {
		dest_x = x;
		dest_y = y;
	}
	else if (justify == JUSTIFY_RIGHT) {
		dest_x = x - calc_width(text);
		dest_y = y;
	}
	else if (justify == JUSTIFY_CENTER) {
		dest_x = x - calc_width(text)/2;
		dest_y = y;
	}
	else {
		printf("ERROR: FontEngine::render() given unhandled 'justify=%d', assuming left\n",
		       justify);
		dest_x = x;
		dest_y = y;
	}

	// render and blit the text
	SDL_Rect dest_rect;
	dest_rect.x = dest_x;
	dest_rect.y = dest_y;

	if (render_blended && target != screen) {
		ttf = TTF_RenderUTF8_Blended(ttfont, text.c_str(), color);

		// preserve alpha transparency of text buffers
		if (ttf != NULL) SDL_gfxBlitRGBA(ttf, NULL, target, &dest_rect);
	}
	else {
		ttf = TTF_RenderUTF8_Solid(ttfont, text.c_str(), color);
		if (ttf != NULL) SDL_BlitSurface(ttf, NULL, target, &dest_rect);
	}

	SDL_FreeSurface(ttf);
	ttf = NULL;
}

/**
 * Word wrap to width
 */
void FontEngine::render(const std::string& text, int x, int y, int justify, SDL_Surface *target, int width, SDL_Color color) {

	string fulltext = text + " ";
	cursor_y = y;
	string next_word;
	stringstream builder;
	stringstream builder_prev;
	char space = 32;
	size_t cursor = 0;
	string swap;

	builder.str("");
	builder_prev.str("");

	next_word = getNextToken(fulltext, cursor, space);

	while(cursor != string::npos) {

		builder << next_word;

		if (calc_width(builder.str()) > width) {
			render(builder_prev.str(), x, cursor_y, justify, target, color);
			cursor_y += getLineHeight();
			builder_prev.str("");
			builder.str("");

			builder << next_word << " ";
		}
		else {
			builder << " ";
			builder_prev.str(builder.str());
		}

		next_word = getNextToken(fulltext, cursor, space); // next word
	}

	render(builder.str(), x, cursor_y, justify, target, color);
	cursor_y += getLineHeight();

}

void FontEngine::renderShadowed(const std::string& text, int x, int y, int justify, SDL_Surface *target, SDL_Color color) {
	render(text, x+1, y+1, justify, target, FONT_BLACK);
	render(text, x, y, justify, target, color);
}

void FontEngine::renderShadowed(const std::string& text, int x, int y, int justify, SDL_Surface *target, int width, SDL_Color color) {
	render(text, x+1, y+1, justify, target, width, FONT_BLACK);
	render(text, x, y, justify, target, width, color);
}

FontEngine::~FontEngine() {
	SDL_FreeSurface(ttf);
	TTF_CloseFont(ttfont);
	TTF_Quit();
}

