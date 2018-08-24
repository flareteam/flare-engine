/*
Copyright © 2011-2012 Clint Bellanger and Thane Brimhall
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2015 Igor Paliychuk
Copyright © 2012-2016 Justin Jacobs

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

#include "FileParser.h"
#include "FontEngine.h"
#include "UtilsParsing.h"

FontStyle::FontStyle()
	: name("")
	, path("")
	, ptsize(0)
	, blend(true)
	, line_height(0)
	, font_height(0) {
}

FontEngine::FontEngine()
	: cursor_y(0)
{
	font_colors.resize(COLOR_COUNT);

	font_colors[COLOR_WHITE] = Color(255,255,255);
	font_colors[COLOR_BLACK] = Color(0,0,0);

	// set the font colors
	// @CLASS FontEngine: Font colors|Description of engine/font_colors.txt
	FileParser infile;
	if (infile.open("engine/font_colors.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			// @ATTR menu_normal|color|Basic menu text color. Recommended: white.
			// @ATTR menu_bonus|color|Positive menu text color. Recommended: green.
			// @ATTR menu_penalty|color|Negative menu text color. Recommended: red.
			// @ATTR widget_normal|color|Basic widget text color. Recommended: white.
			// @ATTR widget_disabled|color|Disabled widget text color. Recommended: grey.
			// @ATTR combat_givedmg|color|Enemy damage text color. Recommended: white.
			// @ATTR combat_takedmg|color|Player damage text color. Recommended: red.
			// @ATTR combat_crit|color|Enemy critical damage text color. Recommended: yellow.
			// @ATTR combat_buff|color|Healing/buff text color. Recommended: green.
			// @ATTR combat_miss|color|Missed attack text color. Recommended: grey.
			// @ATTR requirements_not_met|color|Unmet requirements text color. Recommended: red.
			// @ATTR item_bonus|color|Item bonus text color. Recommended: green.
			// @ATTR item_penalty|color|Item penalty text color. Recommended: red.
			// @ATTR item_flavor|color|Item flavor text color. Recommended: grey.
			// @ATTR hardcore_color_name|color|Permadeath save slot player name color. Recommended: red.
			size_t color_id = stringToFontColor(infile.key);
			if (color_id < COLOR_COUNT)
				font_colors[color_id] = Parse::toRGB(infile.val);
			else
				infile.error("FontEngine: %s is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

}

Color FontEngine::getColor(size_t color_id) {
	if (color_id < font_colors.size())
		return font_colors[color_id];

	// If all else fails, return white;
	return font_colors[COLOR_WHITE];
}

size_t FontEngine::stringToFontColor(const std::string& val) {
	if (val == "menu_normal") return COLOR_MENU_NORMAL;
	else if (val == "menu_bonus") return COLOR_MENU_BONUS;
	else if (val == "menu_penalty") return COLOR_MENU_PENALTY;
	else if (val == "widget_normal") return COLOR_WIDGET_NORMAL;
	else if (val == "widget_disabled") return COLOR_WIDGET_DISABLED;
	else if (val == "combat_givedmg") return COLOR_COMBAT_GIVEDMG;
	else if (val == "combat_takedmg") return COLOR_COMBAT_TAKEDMG;
	else if (val == "combat_crit") return COLOR_COMBAT_CRIT;
	else if (val == "combat_buff") return COLOR_COMBAT_BUFF;
	else if (val == "combat_miss") return COLOR_COMBAT_MISS;
	else if (val == "requirements_not_met") return COLOR_REQUIREMENTS_NOT_MET;
	else if (val == "item_bonus") return COLOR_ITEM_BONUS;
	else if (val == "item_penalty") return COLOR_ITEM_PENALTY;
	else if (val == "item_flavor") return COLOR_ITEM_FLAVOR;
	else if (val == "hardcore_color_name") return COLOR_HARDCORE_NAME;

	// failed to find color
	else return COLOR_COUNT;
}

/**
 * Using the given wrap width, calculate the width and height necessary to display this text
 */
Point FontEngine::calc_size(const std::string& text_with_newlines, int width) {
	char newline = 10;

	std::string text = text_with_newlines;

	// if this contains newlines, recurse
	size_t check_newline = text.find_first_of(newline);
	if (check_newline != std::string::npos) {
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

	std::string next_word;
	std::stringstream builder;
	std::stringstream builder_prev;
	char space = 32;
	size_t cursor = 0;
	std::string fulltext = text + " ";
	std::string long_token;

	builder.str("");
	builder_prev.str("");

	next_word = getNextToken(fulltext, cursor, space);

	while(cursor != std::string::npos) {
		size_t old_cursor = cursor;

		builder << next_word;

		if (calc_width(builder.str()) > width) {

			// this word can't fit on this line, so word wrap
			if (!builder_prev.str().empty()) {
				height += getLineHeight();
				if (calc_width(builder_prev.str()) > max_width) {
					max_width = calc_width(builder_prev.str());
				}
			}

			builder_prev.str("");
			builder.str("");

			long_token = popTokenByWidth(next_word, width);

			if (!long_token.empty()) {
				while (!long_token.empty()) {
					if (calc_width(next_word) > max_width) {
						max_width = calc_width(next_word);
					}
					height += getLineHeight();

					next_word = long_token;
					long_token = popTokenByWidth(next_word, width);

					if (long_token == next_word)
						break;
				}
			}

			builder << next_word << " ";
			builder_prev.str(builder.str());
		}
		else {
			builder <<  " ";
			builder_prev.str(builder.str());
		}

		next_word = getNextToken(fulltext, cursor, space); // next word

		// next token is the same location as the previous token; abort
		if (cursor == old_cursor)
			break;
	}

	builder.str(Parse::trim(builder.str())); //removes whitespace that shouldn't be included in the size
	if (!builder.str().empty())
		height += getLineHeight();
	if (calc_width(builder.str()) > max_width) max_width = calc_width(builder.str());

	// handle blank lines
	if (text_with_newlines == " ")
		height += getLineHeight();

	Point size;
	size.x = max_width;
	size.y = height;
	return size;
}

Rect FontEngine::position(const std::string& text, int x, int y, int justify) {

	Rect dest_rect;
	// calculate actual starting x,y based on justify
	if (justify == JUSTIFY_LEFT) {
		dest_rect.x = x;
		dest_rect.y = y;
	}
	else if (justify == JUSTIFY_RIGHT) {
		dest_rect.x = x - calc_width(text);
		dest_rect.y = y;
	}
	else if (justify == JUSTIFY_CENTER) {
		dest_rect.x = x - calc_width(text)/2;
		dest_rect.y = y;
	}
	else {
		Utils::logError("FontEngine::position() given unhandled 'justify=%d', assuming left",justify);
		dest_rect.x = x;
		dest_rect.y = y;
	}
	return dest_rect;
}

/**
 * Word wrap to width
 */
void FontEngine::render(const std::string& text, int x, int y, int justify, Image *target, int width, const Color& color) {
	if (width == 0) {
		// a width of 0 means we won't try to wrap text
		renderInternal(text, x, y, justify, target, color);
		return;
	}

	std::string fulltext = text + " ";
	cursor_y = y;
	std::string next_word;
	std::stringstream builder;
	std::stringstream builder_prev;
	char space = 32;
	size_t cursor = 0;
	std::string long_token;

	builder.str("");
	builder_prev.str("");

	next_word = getNextToken(fulltext, cursor, space);

	while(cursor != std::string::npos) {
		size_t old_cursor = cursor;

		builder << next_word;

		if (calc_width(builder.str()) > width) {
			if (!builder_prev.str().empty()) {
				renderInternal(builder_prev.str(), x, cursor_y, justify, target, color);
				cursor_y += getLineHeight();
			}
			builder_prev.str("");
			builder.str("");

			long_token = popTokenByWidth(next_word, width);

			if (!long_token.empty()) {
				while (!long_token.empty()) {
					renderInternal(next_word, x, cursor_y, justify, target, color);
					cursor_y += getLineHeight();

					next_word = long_token;
					long_token = popTokenByWidth(next_word, width);

					if (long_token == next_word)
						break;
				}
			}

			builder << next_word << " ";
			builder_prev.str(builder.str());
		}
		else {
			builder << " ";
			builder_prev.str(builder.str());
		}

		next_word = getNextToken(fulltext, cursor, space); // next word

		// next token is the same location as the previous token; abort
		if (cursor == old_cursor)
			break;
	}

	renderInternal(builder.str(), x, cursor_y, justify, target, color);
	cursor_y += getLineHeight();

}

void FontEngine::renderShadowed(const std::string& text, int x, int y, int justify, Image *target, int width, const Color& color) {
	render(text, x+1, y+1, justify, target, width, getColor(COLOR_BLACK));
	render(text, x, y, justify, target, width, color);
}

/*
 * Fits a string, "text", to a pixel "width".
 * The original string is mutated to fit within the width.
 * Returns a string that is the remainder of the original string that could not fit in the width.
 */
std::string FontEngine::popTokenByWidth(std::string& text, int width) {
	size_t new_length = 0;

	for (size_t i = 0; i <= text.length(); ++i) {
		if (i < text.length() && (text[i] & 0xc0) == 0x80)
			continue;

		if (calc_width(text.substr(0, i)) > width)
			break;

		new_length = i;
	}

	if (new_length > 0) {
		std::string ret = text.substr(new_length, text.length());
		text = text.substr(0, new_length);
		return ret;
	}
	else {
		return text;
	}
}

// similar to Parse::popFirstString but does not alter the input string
std::string FontEngine::getNextToken(const std::string& s, size_t &cursor, char separator) {
	size_t seppos = s.find_first_of(separator, cursor);
	if (seppos == std::string::npos) { // not found
		cursor = std::string::npos;
		return "";
	}
	std::string outs = s.substr(cursor, seppos-cursor);
	cursor = seppos+1;
	return outs;
}

