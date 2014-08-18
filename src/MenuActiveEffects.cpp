/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Justin Jacobs
Copyright © 2013 Kurt Rinnert

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
 * MenuActiveEffects
 *
 * Handles the display of active effects (buffs/debuffs)
 */

#include "CommonIncludes.h"
#include "FileParser.h"
#include "MenuActiveEffects.h"
#include "Menu.h"
#include "ModManager.h"
#include "Settings.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

using namespace std;

MenuActiveEffects::MenuActiveEffects(StatBlock *_stats)
	: timer(NULL)
	, stats(_stats)
	, orientation(false) { // horizontal
	// Load config settings
	FileParser infile;
	// @CLASS MenuActiveEffects|Description of menus/activeeffects.txt
	if(infile.open("menus/activeeffects.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR orientation|boolean|True is vertical orientation; False is horizontal orientation.
			if(infile.key == "orientation") {
				orientation = toBool(infile.val);
			}
			else {
				infile.error("MenuActiveEffects: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	loadGraphics();
	align();
}

void MenuActiveEffects::loadGraphics() {
	Image *graphics;
	graphics = render_device->loadImage("images/menus/disabled.png");
	if (graphics) {
		timer = graphics->createSprite();
		graphics->unref();
	}
}

void MenuActiveEffects::renderIcon(int icon_id, int index, int current, int max) {
	if (icon_id > -1) {
		Rect pos,src,overlay;
		if (orientation == 0) {
			pos.x = window_area.x + (index * ICON_SIZE);
			pos.y = window_area.y;
		}
		else if (orientation == 1) {
			pos.x = window_area.x;
			pos.y = window_area.y + (index * ICON_SIZE);
		}

		int columns = icons->getGraphicsWidth() / ICON_SIZE;
		src.x = (icon_id % columns) * ICON_SIZE;
		src.y = (icon_id / columns) * ICON_SIZE;
		src.w = src.h = ICON_SIZE;

		icons->setClip(src);
		icons->setDest(pos);
		render_device->render(icons);

		if (max > 0) {
			overlay.x = 0;
			overlay.y = (ICON_SIZE * current) / max;
			overlay.w = ICON_SIZE;
			overlay.h = ICON_SIZE - overlay.y;

			if (timer) {
				timer->setClip(overlay);
				timer->setDest(pos);
				render_device->render(timer);
			}
		}
	}
}

void MenuActiveEffects::render() {
	int count=-1;

	// Step through the list of effects and render those that are active
	for (unsigned int i=0; i<stats->effects.effect_list.size(); i++) {
		std::string type = stats->effects.effect_list[i].type;
		int icon = stats->effects.effect_list[i].icon;
		int ticks = stats->effects.effect_list[i].ticks;
		int duration = stats->effects.effect_list[i].duration;
		int magnitude = stats->effects.effect_list[i].magnitude;
		int magnitude_max = stats->effects.effect_list[i].magnitude_max;

		if (icon >= 0) count++;

		if (type == "shield")
			renderIcon(icon,count,magnitude,magnitude_max);
		else if (type == "heal" || type == "block")
			renderIcon(icon,count,0,0);
		else if (ticks >= 0 && duration >= 0)
			renderIcon(icon,count,ticks,duration);
	}
}

MenuActiveEffects::~MenuActiveEffects() {
	if (timer)
		delete timer;
}
