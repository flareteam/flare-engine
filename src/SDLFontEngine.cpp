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
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

SDLFontStyle::SDLFontStyle()
	: FontStyle()
	, ttfont(NULL)
	, use_default_style(true)
{
}

SDLFontEngine::SDLFontEngine()
	: FontEngine()
	, active_font(NULL)
{
	// Initiate SDL_ttf
	if(!TTF_WasInit() && TTF_Init()==-1) {
		Utils::logError("SDLFontEngine: TTF_Init: %s", TTF_GetError());
		Utils::logErrorDialog("SDLFontEngine: TTF_Init: %s", TTF_GetError());
		mods->resetModConfig();
		Utils::Exit(2);
	}

	SDLFontStyle temp;
	SDLFontStyle* current = &temp;

	// load the fonts
	// @CLASS SDLFontEngine: Font settings|Description of engine/font_settings.txt
	FileParser infile;
	if (infile.open("engine/font_settings.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while (infile.next()) {
			if (infile.new_section) {
				if (infile.section == "font") {
					temp = SDLFontStyle();
					current = &temp;
				}
				else if (infile.section == "font_fallback") {
					infile.error("FontEngine: Support for 'font_fallback' has been removed.");
					continue;
				}
			}

			if (infile.section != "font")
				continue;

			// if we want to replace a list item by ID, the ID needs to be parsed first
			// but it is not essential if we're just adding to the list, so this is simply a warning
			if (infile.key != "id" && current->name.empty()) {
				infile.error("SDLFontEngine: Expected 'id', but found '%s'.", infile.key.c_str());
			}

			if (infile.key == "id") {
				// @ATTR font.id|string|An identifier used to reference this font.
				bool found_id = false;
				for (size_t i = 0; i < font_styles.size(); ++i) {
					if (font_styles[i].name == infile.val) {
						current = &font_styles[i];
						found_id = true;
					}
				}
				if (!found_id) {
					font_styles.push_back(temp);
					current = &(font_styles.back());
					current->name = infile.val;
				}
			}
			else if (infile.key == "style") {
				// @ATTR font.style|repeatable(["default", predefined_string], filename, int, bool) : Language, Font file, Point size, Blending|Filename, point size, and blend mode of the font to use for this language. Language can be "default" or a 2-letter region code.
				std::string lang = Parse::popFirstString(infile.val);

				if ((lang == "default" && current->use_default_style) || lang == settings->language) {
					if (lang != "default")
						current->use_default_style = false;

					current->path = Parse::popFirstString(infile.val);
					current->ptsize = Parse::popFirstInt(infile.val);
					current->blend = Parse::toBool(Parse::popFirstString(infile.val));
				}
			}
		}
		infile.close();
	}

	// load the font files from the styles
	for (size_t i = 0; i < font_styles.size(); ++i) {
		SDLFontStyle* style = &font_styles[i];

		std::string font_path = mods->locate(style->path);

		// check inside the "fonts/" directory if we can't find our font
		if (!Filesystem::fileExists(mods->locate(style->path))) {
			font_path = mods->locate("fonts/" + style->path);
			if (font_path.empty())
				Utils::logError("FontEngine: Could not find font file: '%s'", style->path.c_str());
		}

		if (!font_path.empty()) {
			style->ttfont = TTF_OpenFont(font_path.c_str(), style->ptsize);
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

	// Attempt to set the default active font
	setFont("font_regular");
	if (!isActiveFontValid()) {
		Utils::logError("FontEngine: Unable to determine default font!");
		Utils::logErrorDialog("FontEngine: Unable to determine default font!");
	}
}

bool SDLFontEngine::isActiveFontValid() {
	return active_font && active_font->ttfont;
}

int SDLFontEngine::getLineHeight() {
	if (!isActiveFontValid())
		return 1;

	return active_font->line_height;
}

int SDLFontEngine::getFontHeight() {
	if (!isActiveFontValid())
		return 1;

	return active_font->font_height;
}

/**
 * For single-line text, just calculate the width
 */
Point SDLFontEngine::calcSize(const std::string& text) {
	if (!isActiveFontValid())
		return Point(1, 1);

	int w, h;
	TTF_SizeUTF8(active_font->ttfont, text.c_str(), &w, &h);

	return Point(w, h);
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
	if (width >= calcSize(text).x)
		return text;

	size_t text_length = text.length();
	size_t ret_length = text_length;
	int total_width = (use_ellipsis ? width - calcSize("...").x : width);

	for (size_t i=text_length; i>0; i--) {
		if (use_ellipsis) {
			if (total_width < calcSize(text.substr(0,ret_length)).x)
				ret_length = i;
			else
				break;
		}
		else if (left_pos < text_length - ret_length) {
			if (total_width < calcSize(text.substr(left_pos,ret_length)).x)
				ret_length = i;
			else
				break;
		}
		else {
			if (total_width < calcSize(text.substr(text_length-ret_length)).x)
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
		if (font_styles[i].ttfont != NULL && font_styles[i].name == _font) {
			active_font = &(font_styles[i]);
			return;
		}
	}

	// Unable to find a matching font. Try the first available font style instead
	for (unsigned int i=0; i<font_styles.size(); i++) {
		if (font_styles[i].ttfont != NULL) {
			Utils::logError("FontEngine: Invalid font '%s'. Falling back to '%s'.", _font.c_str(), font_styles[i].name.c_str());
			active_font = &(font_styles[i]);
			return;
		}
	}

	Utils::logError("FontEngine: Invalid font '%s'. No fallback available.", _font.c_str());
}

/**
 * Render the given text at (x,y) on the target image.
 * Justify is left, right, or center
 */
void SDLFontEngine::renderInternal(const std::string& text, int x, int y, int justify, Image *target, const Color& color) {
	if (!isActiveFontValid() || text.empty())
		return;

	Image *graphics;

	Rect dest_rect = position(text, x, y, justify);

	// Render text into target
	// We render the same thing twice because blending with itself produces visually clearer text, especially on noisy backgrounds
	graphics = render_device->renderTextToImage(active_font, text, color, active_font->blend);
	if (graphics) {
		if (target) {
			Rect clip;
			clip.w = graphics->getWidth();
			clip.h = graphics->getHeight();
			render_device->renderToImage(graphics, clip, target, dest_rect);
			render_device->renderToImage(graphics, clip, target, dest_rect);
		}
		else {
			// no target, so just render to the screen
			Sprite* temp_sprite = graphics->createSprite();
			if (temp_sprite) {
				temp_sprite->setDestFromRect(dest_rect);
				render_device->render(temp_sprite);
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

