/*
Copyright © 2011-2012 Clint Bellanger and Thane Brimhall
Copyright © 2013 Kurt Rinnert
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

/*
 * class SDLFontEngine
 */

#include "CommonIncludes.h"
#include "SDLFontEngine.h"
#include "FileParser.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsParsing.h"

SDLFontStyle::SDLFontStyle() : FontStyle(), ttfont(NULL) {
}

SDLFontEngine::SDLFontEngine() : FontEngine(), active_font(NULL) {
	// Initiate SDL_ttf
	if(!TTF_WasInit() && TTF_Init()==-1) {
		logError("SDLFontEngine: TTF_Init: %s", TTF_GetError());
		Exit(2);
	}

	// load the fonts
	// @CLASS SDLFontEngine: Font settings|Description of engine/font_settings.txt
	FileParser infile;
	if (infile.open("engine/font_settings.txt")) {
		while (infile.next()) {
			if (infile.new_section) {
				SDLFontStyle f;
				f.name = infile.section;
				font_styles.push_back(f);
			}

			if (font_styles.empty()) continue;

			SDLFontStyle *style = &(font_styles.back());
			if ((infile.key == "default" && style->path == "") || infile.key == LANGUAGE) {
				// @ATTR $STYLE.default, $STYLE.$LANGUAGE|filename (string), point size (integer), blending (boolean)|Filename, point size, and blend mode of the font to use for this language. $STYLE can be something like "font_normal" or "font_bold". $LANGUAGE can be a 2-letter region code.
				style->path = popFirstString(infile.val);
				style->ptsize = popFirstInt(infile.val);
				style->blend = toBool(popFirstString(infile.val));
				style->ttfont = TTF_OpenFont(mods->locate("fonts/" + style->path).c_str(), style->ptsize);
				if(style->ttfont == NULL) {
					logError("FontEngine: TTF_OpenFont: %s", TTF_GetError());
				}
				else {
					int lineskip = TTF_FontLineSkip(style->ttfont);
					style->line_height = lineskip;
					style->font_height = lineskip;
				}
			}
		}
		infile.close();
	}

	// set the font colors
	Color color;
	if (infile.open("engine/font_colors.txt")) {
		while (infile.next()) {
			// @ATTR menu_normal, menu_bonus, menu_penalty, widget_normal, widget_disabled|r (integer), g (integer), b (integer)|Colors for menus and widgets
			// @ATTR combat_givedmg, combat_takedmg, combat_crit, combat_buff, combat_miss|r (integer), g (integer), b (integer)|Colors for combat text
			// @ATTR requirements_not_met, item_bonus, item_penalty, item_flavor|r (integer), g (integer), b (integer)|Colors for tooltips
			// @ATTR item_$QUALITY|r (integer), g (integer), b (integer)|Colors for item quality. $QUALITY should match qualities used in items/items.txt
			color_map[infile.key] = toRGB(infile.val);
		}
		infile.close();
	}

	// Attempt to set the default active font
	setFont("font_regular");
	if (!active_font) {
		logError("FontEngine: Unable to determine default font!");
		Exit(1);
	}
}

int SDLFontEngine::getLineHeight() {
	return active_font->line_height;
}

int SDLFontEngine::getFontHeight() {
	return active_font->font_height;
}

/**
 * For single-line text, just calculate the width
 */
int SDLFontEngine::calc_width(const std::string& text) {
	int w, h;
	TTF_SizeUTF8(active_font->ttfont, text.c_str(), &w, &h);
	return w;
}

/**
 * Fit a string of text into a pixel width
 * use_ellipsis determines how the returned string will appear
 * Example with "Hello World" (let's assume a monospace font and a width that can fit 6 characters):
 * use_ellipsis == true: "Hello ..."
 * use_ellipsis == false: " World"
 */
std::string SDLFontEngine::trimTextToWidth(const std::string& text, const int width, const bool use_ellipsis) {
	if (width >= calc_width(text))
		return text;

	size_t text_length = text.length();
	size_t ret_length = text_length;
	int total_width = (use_ellipsis ? width - calc_width("...") : width);

	for (size_t i=text_length; i>0; i--) {
		if (use_ellipsis) {
			if (total_width < calc_width(text.substr(0,ret_length)))
				ret_length = i;
			else
				break;
		}
		else {
			if (total_width < calc_width(text.substr(text_length-ret_length)))
				ret_length = i;
			else
				break;
		}
	}

	if (!use_ellipsis) {
		return text.substr(text_length-ret_length);
	}
	else {
		if (text_length <= 3)
			return std::string("...");

		if (text_length-ret_length < 3)
			ret_length = text_length-3;

		std::string ret_str = text.substr(0,ret_length);
		ret_str = ret_str + '.' + '.' + '.';
		return ret_str;
	}
}

void SDLFontEngine::setFont(const std::string& _font) {
	for (unsigned int i=0; i<font_styles.size(); i++) {
		if (font_styles[i].name == _font) {
			active_font = &(font_styles[i]);
			return;
		}
	}
}

/**
 * Render the given text at (x,y) on the target image.
 * Justify is left, right, or center
 */
void SDLFontEngine::render(const std::string& text, int x, int y, int justify, Image *target, const Color& color) {
	Rect clip;
	Image *graphics;
	Sprite *temp;

	Rect dest_rect = position(text, x, y, justify);

	// Render text directly onto screen
	if (!target) {
		render_device->renderText(active_font, text, color, dest_rect);
		return;
	}

	// Render text into target Image
	graphics = render_device->renderTextToImage(active_font, text, color, active_font->blend);
	if (!graphics) return;
	temp = graphics->createSprite();
	graphics->unref();

	// Render text graphics into target
	clip = temp->getClip();
	render_device->renderToImage(temp->getGraphics(), clip, target, dest_rect);

	// text is cached, we can free temp resource
	delete temp;
}

SDLFontEngine::~SDLFontEngine() {
	for (unsigned int i=0; i<font_styles.size(); ++i) TTF_CloseFont(font_styles[i].ttfont);
	TTF_Quit();
}

