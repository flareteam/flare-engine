/*
Copyright © 2011-2012 Clint Bellanger and Thane Brimhall
Copyright © 2013 Kurt Rinnert
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

/*
 * class SDLFontEngine
 */

#include "CommonIncludes.h"
#include "SDLFontEngine.h"
#include "FileParser.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsParsing.h"

SDLFontStyle::SDLFontStyle() : FontStyle(), ttfont(NULL) {
}

SDLFontEngine::SDLFontEngine() : FontEngine(), active_font(NULL) {
	// Initiate SDL_ttf
	if(!TTF_WasInit() && TTF_Init()==-1) {
		Utils::logError("SDLFontEngine: TTF_Init: %s", TTF_GetError());
		Utils::logErrorDialog("SDLFontEngine: TTF_Init: %s", TTF_GetError());
		mods->resetModConfig();
		Utils::Exit(2);
	}

	// load the fonts
	// @CLASS SDLFontEngine: Font settings|Description of engine/font_settings.txt
	FileParser infile;
	if (infile.open("engine/font_settings.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section && infile.section == "font") {
				font_styles.push_back(SDLFontStyle());
			}
			else if (infile.section == "font_fallback") {
				if (infile.new_section)
					infile.error("FontEngine: Support for 'font_fallback' has been removed.");
				continue;
			}

			SDLFontStyle *style;

			if (font_styles.empty())
				continue;

			style = &(font_styles.back());

			if (infile.key == "id") {
				// @ATTR font.id|string|An identifier used to reference this font.
				style->name = infile.val;
			}
			else if (infile.key == "style") {
				// @ATTR font.style|repeatable(["default", predefined_string], filename, int, bool) : Language, Font file, Point size, Blending|Filename, point size, and blend mode of the font to use for this language. Language can be "default" or a 2-letter region code.

				std::string lang = Parse::popFirstString(infile.val);

				if ((lang == "default" && style->path == "") || lang == settings->language) {
					style->path = Parse::popFirstString(infile.val);
					style->ptsize = Parse::popFirstInt(infile.val);
					style->blend = Parse::toBool(Parse::popFirstString(infile.val));

					style->ttfont = TTF_OpenFont(mods->locate("fonts/" + style->path).c_str(), style->ptsize);
					if(style->ttfont == NULL) {
						Utils::logError("FontEngine: TTF_OpenFont: %s", TTF_GetError());
					}
					else {
						int lineskip = TTF_FontLineSkip(style->ttfont);
						style->line_height = lineskip;
						style->font_height = lineskip;
					}
				}
			}
		}
		infile.close();
	}

	// Attempt to set the default active font
	setFont("font_regular");
	if (!active_font) {
		Utils::logError("FontEngine: Unable to determine default font!");
		Utils::logErrorDialog("FontEngine: Unable to determine default font!");
		mods->resetModConfig();
		Utils::Exit(1);
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
 *
 * left_pos is only used when use_ellipsis is false.
 * It ensures that this character is visible, chopping the end of the string if needed.
 */
std::string SDLFontEngine::trimTextToWidth(const std::string& text, const int width, const bool use_ellipsis, size_t left_pos) {
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
		else if (left_pos < text_length - ret_length) {
			if (total_width < calc_width(text.substr(left_pos,ret_length)))
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

		while (ret_length > 0 && ((text[ret_length] & 0xc0) == 0x80)) {
			ret_length--;
		}
	}

	if (!use_ellipsis) {
		if (left_pos < text_length - ret_length)
			return text.substr(left_pos, ret_length);
		else
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
void SDLFontEngine::renderInternal(const std::string& text, int x, int y, int justify, Image *target, const Color& color) {
	if (text.empty())
		return;

	Image *graphics;

	Rect dest_rect = position(text, x, y, justify);

	// Render text into target
	graphics = render_device->renderTextToImage(active_font, text, color, active_font->blend);
	if (graphics) {
		if (target) {
			Rect clip;
			clip.w = graphics->getWidth();
			clip.h = graphics->getHeight();
			render_device->renderToImage(graphics, clip, target, dest_rect);
		}
		else {
			Sprite* temp_sprite = graphics->createSprite();
			if (temp_sprite) {
				temp_sprite->setDestFromRect(dest_rect);
				render_device->render(temp_sprite);
				delete temp_sprite;
			}
		}

		// text is cached, we can free temp resource
		graphics->unref();
	}
}

SDLFontEngine::~SDLFontEngine() {
	for (unsigned int i=0; i<font_styles.size(); ++i) TTF_CloseFont(font_styles[i].ttfont);
	TTF_Quit();
}

