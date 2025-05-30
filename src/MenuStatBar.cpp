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

#include "Avatar.h"
#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FontEngine.h"
#include "InputState.h"
#include "Menu.h"
#include "MenuExit.h"
#include "MenuManager.h"
#include "MenuStatBar.h"
#include "MessageEngine.h"
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

MenuStatBar::MenuStatBar(short _type, size_t _resource_stat_index)
	: bar(NULL)
	, label(new WidgetLabel())
	, enabled(true)
	, orientation(HORIZONTAL)
	, custom_text_pos(false) // label will be placed in the middle of the bar
	, custom_string("")
	, bar_gfx("")
	, bar_gfx_background("")
	, type(_type)
	, resource_stat_index(_resource_stat_index)
	, bar_fill_offset()
	, bar_fill_size(-1, -1)
{
	std::string type_filename;
	if (type == TYPE_HP)
		type_filename = "menus/hp.txt";
	else if (type == TYPE_MP)
		type_filename = "menus/mp.txt";
	else if (type == TYPE_XP)
		type_filename = "menus/xp.txt";
	else if (type == TYPE_RESOURCE_STAT)
		type_filename = eset->resource_stats.list[resource_stat_index].menu_filename;

	if (type == TYPE_XP) {
		stat_min.Unsigned = 0;
		stat_cur.Unsigned = 0;
		stat_cur_prev.Unsigned = 0;
		stat_max.Unsigned = 0;
	}
	else {
		stat_min.Float = 0;
		stat_cur.Float = 0;
		stat_cur_prev.Float = 0;
		stat_max.Float = 0;
	}

	// Load config settings
	FileParser infile;
	// @CLASS MenuStatBar|Description of menus/hp.txt, menus/mp.txt, menus/xp.txt
	if(!type_filename.empty() && infile.open(type_filename, FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
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
			// @ATTR bar_fill_offset|point|Offset of the bar's fill graphics relative to the bar_pos X/Y.
			else if (infile.key == "bar_fill_offset") {
				bar_fill_offset = Parse::toPoint(infile.val);
			}
			// @ATTR bar_fill_size|int, int : Width, Height|Size of the bar's fill graphics. If not defined, the width/height of bar_pos is used.
			else if (infile.key == "bar_fill_size") {
				bar_fill_size = Parse::toPoint(infile.val);
			}
			// @ATTR enabled|bool|Determines if the bar will be rendered. Disable the bar completely by setting this to false.
			else if (infile.key == "enabled") {
				enabled = Parse::toBool(infile.val);
			}
			else {
				infile.error("MenuStatBar: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// default to bar_pos size if bar_fill_size is undefined
	if (bar_fill_size.x == -1 || bar_fill_size.y == -1) {
		bar_fill_size.x = bar_pos.w;
		bar_fill_size.y = bar_pos.h;
	}

	loadGraphics();

	align();
}

void MenuStatBar::loadGraphics() {
	if (!enabled)
		return;

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

void MenuStatBar::update() {
	if (!enabled)
		return;

	if (type == TYPE_XP) {
		stat_cur_prev.Unsigned = stat_cur.Unsigned; // save previous value
		stat_min.Unsigned = 0;
		stat_cur.Unsigned = pc->stats.xp - eset->xp.getLevelXP(pc->stats.level);
		stat_max.Unsigned = eset->xp.getLevelXP(pc->stats.level + 1) - eset->xp.getLevelXP(pc->stats.level);

		if (pc->stats.level == eset->xp.getMaxLevel()) {
			custom_string = msg->getv("XP: %lu", pc->stats.xp);
		}
		else {
			custom_string = msg->getv("XP: %lu/%lu", stat_cur.Unsigned, stat_max.Unsigned);
		}
	}
	else if (type == TYPE_HP) {
		stat_cur_prev.Float = stat_cur.Float; // save previous value
		stat_min.Float = 0;
		stat_cur.Float = pc->stats.hp;
		stat_max.Float = pc->stats.get(Stats::HP_MAX);
	}
	else if (type == TYPE_MP) {
		stat_cur_prev.Float = stat_cur.Float; // save previous value
		stat_min.Float = 0;
		stat_cur.Float = pc->stats.mp;
		stat_max.Float = pc->stats.get(Stats::MP_MAX);
	}
	else if (type == TYPE_RESOURCE_STAT) {
		stat_cur_prev.Float = stat_cur.Float; // save previous value
		stat_min.Float = 0;
		stat_cur.Float = pc->stats.resource_stats[resource_stat_index];
		stat_max.Float = pc->stats.getResourceStat(resource_stat_index, EngineSettings::ResourceStats::STAT_BASE);
	}
}

bool MenuStatBar::disappear() {
	if (!enabled)
		return true;

	if (timeout.getDuration() > 0 && settings->statbar_autohide) {
		if (type == TYPE_XP) {
			// XP bar disappears when value is not changing
			if (stat_cur_prev.Unsigned != stat_cur.Unsigned) {
				timeout.reset(Timer::BEGIN);
			}
		}
		else {
			// HP and MP bars disappear when full
			if (stat_cur.Float != stat_max.Float) {
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

	int bar_length = 0;

	if (type == TYPE_XP) {
		unsigned long stat_cur_clamped = std::min(stat_cur.Unsigned, stat_max.Unsigned);
		unsigned long normalized_cur = stat_cur_clamped - std::min(stat_cur_clamped, stat_min.Unsigned);
		unsigned long normalized_max = stat_max.Unsigned - std::min(stat_max.Unsigned, stat_min.Unsigned);

		unsigned long bar_fill_size_oriented = 0;
		if (orientation == HORIZONTAL)
			bar_fill_size_oriented = static_cast<unsigned long>(bar_fill_size.x);
		else if (orientation == VERTICAL)
			bar_fill_size_oriented = static_cast<unsigned long>(bar_fill_size.y);

		bar_length = static_cast<int>((normalized_max == 0) ? 0 : (normalized_cur * bar_fill_size_oriented) / normalized_max);
		if (bar_length == 0 && normalized_cur > 0)
			bar_length = 1;
	}
	else {
		float stat_cur_clamped = std::min(stat_cur.Float, stat_max.Float);
		float normalized_cur = stat_cur_clamped - std::min(stat_cur_clamped, stat_min.Float);
		float normalized_max = stat_max.Float - std::min(stat_max.Float, stat_min.Float);

		float bar_fill_size_oriented = 0;
		if (orientation == HORIZONTAL)
			bar_fill_size_oriented = static_cast<float>(bar_fill_size.x);
		else if (orientation == VERTICAL)
			bar_fill_size_oriented = static_cast<float>(bar_fill_size.y);

		bar_length = static_cast<int>((normalized_max == 0) ? 0 : (normalized_cur * bar_fill_size_oriented) / normalized_max);
		if (bar_length == 0 && normalized_cur > 0)
			bar_length = 1;
	}

	// draw bar progress based on orientation
	if (orientation == HORIZONTAL) {
		src.x = 0;
		src.y = 0;
		src.w = bar_length;
		src.h = bar_fill_size.y;
		dest.x = bar_dest.x + bar_fill_offset.x;
		dest.y = bar_dest.y + bar_fill_offset.y;
	}
	else if (orientation == VERTICAL) {
		src.x = 0;
		src.y = bar_fill_size.y - bar_length;
		src.w = bar_fill_size.x;
		src.h = bar_length;
		dest.x = bar_dest.x + bar_fill_offset.x;
		dest.y = bar_dest.y + bar_fill_offset.y + src.y;
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
			else if (type == TYPE_XP)
				// TYPE_XP uses a custom string, so this won't apply in normal circumstances
				ss << stat_cur.Unsigned << "/" << stat_max.Unsigned;
			else
				ss << Utils::floatToString(stat_cur.Float, eset->number_format.player_statbar) << "/" << Utils::floatToString(stat_max.Float, eset->number_format.player_statbar);

			label->setText(ss.str());
			label->setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

			if (custom_text_pos) {
				label->setPos(bar_dest.x + text_pos.x, bar_dest.y + text_pos.y);
				label->setJustify(text_pos.justify);
				label->setVAlign(text_pos.valign);
				label->setFont(text_pos.font_style);
			}
			else {
				if (bar) {
					// position bar text relative to bar fill if possible
					label->setPos(dest.x + bar_fill_size.x/2, dest.y + bar_fill_size.y/2);
				}
				else {
					label->setPos(bar_dest.x + bar_pos.w/2, bar_dest.y + bar_pos.h/2);
				}
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
