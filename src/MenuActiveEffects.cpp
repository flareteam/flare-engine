/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2015 Justin Jacobs
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

			// @ATTR orientation|bool|True is vertical orientation; False is horizontal orientation.
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

void MenuActiveEffects::logic() {
	effect_icons.clear();

	for (size_t i = 0; i < stats->effects.effect_list.size(); ++i) {
		if (stats->effects.effect_list[i].icon == -1)
			continue;

		const Effect &ed = stats->effects.effect_list[i];
		EffectIcon ei;
		ei.icon = ed.icon;
		ei.name = ed.name;
		ei.type = ed.type;

		// icon position
		if (orientation == 0) {
			ei.pos.x = window_area.x + (static_cast<int>(effect_icons.size()) * ICON_SIZE);
			ei.pos.y = window_area.y;
		}
		else if (orientation == 1) {
			ei.pos.x = window_area.x;
			ei.pos.y = window_area.y + (static_cast<int>(effect_icons.size()) * ICON_SIZE);
		}
		ei.pos.w = ei.pos.h = ICON_SIZE;

		// timer overlay
		ei.overlay.x = 0;
		ei.overlay.w = ICON_SIZE;

		if (ed.type == EFFECT_SHIELD) {
			ei.overlay.y = (ICON_SIZE * ed.magnitude) / ed.magnitude_max;
			ei.current = ed.magnitude;
			ei.max = ed.magnitude_max;
		}
		else if (ed.type == EFFECT_HEAL) {
			ei.overlay.y = ICON_SIZE;
			// current and max are ignored
		}
		else {
			ei.overlay.y = (ICON_SIZE * ed.ticks) / ed.duration;
			ei.current = ed.ticks;
			ei.max = ed.duration;
		}
		ei.overlay.h = ICON_SIZE - ei.overlay.y;

		effect_icons.push_back(ei);
	}

	if (orientation == 0) {
		window_area.w = static_cast<int>(effect_icons.size()) * ICON_SIZE;
		window_area.h = ICON_SIZE;
	}
	else if (orientation == 1) {
		window_area.w = ICON_SIZE;
		window_area.h = static_cast<int>(effect_icons.size()) * ICON_SIZE;
	}
	Menu::align();
}

void MenuActiveEffects::render() {
	for (size_t i = 0; i < effect_icons.size(); ++i) {
		Point icon_pos(effect_icons[i].pos.x, effect_icons[i].pos.y);
		icons->setIcon(effect_icons[i].icon, icon_pos);
		icons->render();

		if (timer) {
			timer->setClip(effect_icons[i].overlay);
			timer->setDest(effect_icons[i].pos);
			render_device->render(timer);
		}
	}
}

TooltipData MenuActiveEffects::checkTooltip(const Point& mouse) {
	TooltipData tip;

	for (size_t i = 0; i < effect_icons.size(); ++i) {
		if (!effect_icons[i].name.empty() && isWithinRect(effect_icons[i].pos, mouse)) {
			tip.addText(msg->get(effect_icons[i].name));

			std::stringstream ss;
			if (effect_icons[i].type == EFFECT_SHIELD) {
				ss << "(" << effect_icons[i].current << "/" << effect_icons[i].max << ")";
				tip.addText(ss.str());
			}
			else if (effect_icons[i].type != EFFECT_HEAL) {
				ss << msg->get("Remaining:") << " " << getDurationString(effect_icons[i].current, 1);
				tip.addText(ss.str());
			}

			break;
		}
	}

	return tip;
}

MenuActiveEffects::~MenuActiveEffects() {
	if (timer)
		delete timer;
}
