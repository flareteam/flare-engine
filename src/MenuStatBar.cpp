/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Justin Jacobs
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

/**
 * MenuStatBar
 *
 * Handles the display of a status bar
 */

#include "CommonIncludes.h"
#include "Menu.h"
#include "MenuStatBar.h"
#include "ModManager.h"
#include "Settings.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "WidgetLabel.h"
#include "FileParser.h"
#include "UtilsParsing.h"
#include "UtilsFileSystem.h"

MenuStatBar::MenuStatBar(const std::string& type)
	: bar(NULL)
	, stat_cur(0)
	, stat_max(0)
	, orientation(0) // horizontal
	, custom_text_pos(false) // label will be placed in the middle of the bar
	, custom_string("")
	, bar_gfx("")
	, bar_gfx_background("")
{

	label = new WidgetLabel();

	// Load config settings
	FileParser infile;
	// @CLASS MenuStatBar|Description of menus/hp.txt, menus/mp.txt, menus/xp.txt
	if(infile.open("menus/"+type+".txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR bar_pos|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the bar graphics.
			if(infile.key == "bar_pos") {
				bar_pos = toRect(infile.val);
			}
			// @ATTR text_pos|label|Position of the "$CURRENT/$TOTAL" text.
			else if(infile.key == "text_pos") {
				custom_text_pos = true;
				text_pos = eatLabelInfo(infile.val);
			}
			// @ATTR orientation|boolean|True is vertical orientation; false is horizontal.
			else if(infile.key == "orientation") {
				orientation = toBool(infile.val);
			}
			// @ATTR bar_gfx|string|Filename of the image to use for the "fill" of the bar.
			else if (infile.key == "bar_gfx") {
				bar_gfx = infile.val;
			}
			// @ATTR bar_gfx_background|string|Filename of the image to use for the base of the bar.
			else if (infile.key == "bar_gfx_background") {
				bar_gfx_background = infile.val;
			}
			else {
				infile.error("MenuStatBar: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	loadGraphics();

	color_normal = font->getColor("menu_normal");

	align();
}

void MenuStatBar::loadGraphics() {
	Image *graphics;

	if (bar_gfx_background != "") {
		setBackground(bar_gfx_background);
	}

	if (bar_gfx != "") {
		graphics = render_device->loadImage(bar_gfx);
		if (graphics) {
			bar = graphics->createSprite();
			graphics->unref();
		}
	}
}

void MenuStatBar::update(unsigned long _stat_cur, unsigned long _stat_max, const Point& _mouse, const std::string& _custom_string) {
	if (_custom_string != "") custom_string = _custom_string;
	mouse = _mouse;
	stat_cur = _stat_cur;
	stat_max = _stat_max;
}

void MenuStatBar::render() {
	Rect src;
	Rect dest;

	// position elements based on the window position
	Rect bar_dest = bar_pos;
	bar_dest.x = bar_pos.x+window_area.x;
	bar_dest.y = bar_pos.y+window_area.y;

	// draw bar background
	dest.x = bar_dest.x;
	dest.y = bar_dest.y;
	src.x = 0;
	src.y = 0;
	src.w = bar_pos.w;
	src.h = bar_pos.h;
	setBackgroundClip(src);
	setBackgroundDest(dest);
	Menu::render();

	// draw bar progress based on orientation
	if (orientation == 0) {
		unsigned long bar_length = (stat_max == 0) ? 0 : (stat_cur * static_cast<unsigned long>(bar_pos.w)) / stat_max;
		src.x = 0;
		src.y = 0;
		src.w = static_cast<int>(bar_length);
		src.h = bar_pos.h;
		dest.x = bar_dest.x;
		dest.y = bar_dest.y;
	}
	else if (orientation == 1) {
		unsigned long bar_length = (stat_max == 0) ? 0 : (stat_cur * static_cast<unsigned long>(bar_pos.h)) / stat_max;
		src.x = 0;
		src.y = bar_pos.h-static_cast<int>(bar_length);
		src.w = bar_pos.w;
		src.h = static_cast<int>(bar_length);
		dest.x = bar_dest.x;
		dest.y = bar_dest.y+src.y;
	}

	if (bar) {
		bar->setClip(src);
		bar->setDest(dest);
		render_device->render(bar);
	}

	// if mouseover, draw text
	if (!text_pos.hidden) {
		if (custom_text_pos)
			label->set(bar_dest.x+text_pos.x, bar_dest.y+text_pos.y, text_pos.justify, text_pos.valign, "", color_normal, text_pos.font_style);
		else
			label->set(bar_dest.x+bar_pos.w/2, bar_dest.y+bar_pos.h/2, JUSTIFY_CENTER, VALIGN_CENTER, "", color_normal);

		if (STATBAR_LABELS || isWithin(bar_dest,mouse)) {
			std::stringstream ss;
			if (custom_string != "")
				ss << custom_string;
			else
				ss << stat_cur << "/" << stat_max;
			label->set(ss.str());
			label->render();
		}
	}
}

MenuStatBar::~MenuStatBar() {
	if (bar) delete bar;
	delete label;
}
