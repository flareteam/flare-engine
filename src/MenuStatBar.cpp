/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Justin Jacobs
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
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

/**
 * MenuStatBar
 *
 * Handles the display of a status bar
 */

#include "CommonIncludes.h"
#include "FontEngine.h"
#include "InputState.h"
#include "Menu.h"
#include "MenuExit.h"
#include "MenuManager.h"
#include "MenuStatBar.h"
#include "ModManager.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "WidgetLabel.h"
#include "FileParser.h"
#include "UtilsParsing.h"
#include "UtilsFileSystem.h"

MenuStatBar::MenuStatBar(short _type)
	: bar(NULL)
	, label(new WidgetLabel())
	, stat_min(0)
	, stat_cur(0)
	, stat_cur_prev(0)
	, stat_max(0)
	, orientation(HORIZONTAL)
	, custom_text_pos(false) // label will be placed in the middle of the bar
	, custom_string("")
	, bar_gfx("")
	, bar_gfx_background("")
	, type(_type)
{
	std::string type_filename;
	if (type == TYPE_HP)
		type_filename = "hp";
	else if (type == TYPE_MP)
		type_filename = "mp";
	else if (type == TYPE_XP)
		type_filename = "xp";

	// Load config settings
	FileParser infile;
	// @CLASS MenuStatBar|Description of menus/hp.txt, menus/mp.txt, menus/xp.txt
	if(!type_filename.empty() && infile.open("menus/" + type_filename + ".txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR bar_pos|rectangle|Position and dimensions of the bar graphics.
			if(infile.key == "bar_pos") {
				bar_pos = Parse::toRect(infile.val);
			}
			// @ATTR text_pos|label|Position of the text displaying the current value of the relevant stat.
			else if(infile.key == "text_pos") {
				custom_text_pos = true;
				text_pos = Parse::popLabelInfo(infile.val);
			}
			// @ATTR orientation|bool|True is vertical orientation; false is horizontal.
			else if(infile.key == "orientation") {
				orientation = Parse::toBool(infile.val);
			}
			// @ATTR bar_gfx|filename|Filename of the image to use for the "fill" of the bar.
			else if (infile.key == "bar_gfx") {
				bar_gfx = infile.val;
			}
			// @ATTR bar_gfx_background|filename|Filename of the image to use for the base of the bar.
			else if (infile.key == "bar_gfx_background") {
				bar_gfx_background = infile.val;
			}
			// @ATTR hide_timeout|duration|Hide HP and MP bar if full mana or health, after given amount of seconds; Hide XP bar if no changes in XP points for given amount of seconds. 0 disable hiding.
			else if (infile.key == "hide_timeout") {
				timeout.setDuration(Parse::toDuration(infile.val));
			}
			else {
				infile.error("MenuStatBar: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	loadGraphics();

	align();
}

void MenuStatBar::loadGraphics() {
	Image *graphics;

	if (bar_gfx_background != "") {
		setBackground(bar_gfx_background);
	}

	if (bar_gfx != "") {
		graphics = render_device->loadImage(bar_gfx, RenderDevice::ERROR_NORMAL);
		if (graphics) {
			bar = graphics->createSprite();
			graphics->unref();
		}
	}
}

void MenuStatBar::update(unsigned long _stat_min, unsigned long _stat_cur, unsigned long _stat_max) {
	stat_cur_prev = stat_cur; // save previous value
	stat_min = _stat_min;
	stat_cur = _stat_cur;
	stat_max = _stat_max;
}

void MenuStatBar::setCustomString(const std::string& _custom_string) {
	custom_string = _custom_string;
}

bool MenuStatBar::disappear() {
	if (timeout.getDuration() > 0 && settings->statbar_autohide) {
		if (type == TYPE_HP || type == TYPE_MP) {
			// HP and MP bars disappear when full
			if (stat_cur != stat_max) {
				timeout.reset(Timer::BEGIN);
			}
		} else if (type == TYPE_XP) {
			// XP bar disappears when value is not changing
			if (stat_cur_prev != stat_cur) {
				timeout.reset(Timer::BEGIN);
			}
		}

		timeout.tick();
		if (timeout.isEnd())
			return true;
	}
	return false;
}

void MenuStatBar::render() {

	if (disappear()) return;

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

	unsigned long stat_cur_clamped = std::min(stat_cur, stat_max);
	unsigned long normalized_cur = stat_cur_clamped - std::min(stat_cur_clamped, stat_min);
	unsigned long normalized_max = stat_max - std::min(stat_max, stat_min);

	// draw bar progress based on orientation
	if (orientation == HORIZONTAL) {
		unsigned long bar_length = (normalized_max == 0) ? 0 : (normalized_cur * static_cast<unsigned long>(bar_pos.w)) / normalized_max;
		src.x = 0;
		src.y = 0;
		src.w = static_cast<int>(bar_length);
		src.h = bar_pos.h;
		dest.x = bar_dest.x;
		dest.y = bar_dest.y;
	}
	else if (orientation == VERTICAL) {
		unsigned long bar_length = (normalized_max == 0) ? 0 : (normalized_cur * static_cast<unsigned long>(bar_pos.h)) / normalized_max;
		src.x = 0;
		src.y = bar_pos.h-static_cast<int>(bar_length);
		src.w = bar_pos.w;
		src.h = static_cast<int>(bar_length);
		dest.x = bar_dest.x;
		dest.y = bar_dest.y+src.y;
	}

	if (bar) {
		bar->setClipFromRect(src);
		bar->setDestFromRect(dest);
		render_device->render(bar);
	}

	// if mouseover, draw text
	if (!text_pos.hidden) {

		if (settings->statbar_labels || (inpt->usingMouse() && Utils::isWithinRect(bar_dest, inpt->mouse) && !menu->exit->visible)) {
			std::stringstream ss;
			if (!custom_string.empty())
				ss << custom_string;
			else
				ss << stat_cur << "/" << stat_max;

			label->setText(ss.str());
			label->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

			if (custom_text_pos) {
				label->setPos(bar_dest.x + text_pos.x, bar_dest.y + text_pos.y);
				label->setJustify(text_pos.justify);
				label->setVAlign(text_pos.valign);
				label->setFont(text_pos.font_style);
			}
			else {
				label->setPos(bar_dest.x + bar_pos.w/2, bar_dest.y + bar_pos.h/2);
				label->setJustify(FontEngine::JUSTIFY_CENTER);
				label->setVAlign(LabelInfo::VALIGN_CENTER);
			}
			label->render();
		}
	}
}

MenuStatBar::~MenuStatBar() {
	if (bar) delete bar;
	delete label;
}
