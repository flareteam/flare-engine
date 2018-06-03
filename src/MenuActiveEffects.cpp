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
#include "IconManager.h"
#include "MenuActiveEffects.h"
#include "Menu.h"
#include "ModManager.h"
#include "MessageEngine.h"
#include "RenderDevice.h"
#include "Settings.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "TooltipData.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"

MenuActiveEffects::MenuActiveEffects(StatBlock *_stats)
	: timer(NULL)
	, stats(_stats)
	, is_vertical(false)
{
	// Load config settings
	FileParser infile;
	// @CLASS MenuActiveEffects|Description of menus/activeeffects.txt
	if(infile.open("menus/activeeffects.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR vertical|bool|True is vertical orientation; False is horizontal orientation.
			if(infile.key == "vertical") {
				is_vertical = toBool(infile.val);
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
	graphics = render_device->loadImage("images/menus/disabled.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		timer = graphics->createSprite();
		graphics->unref();
	}
}

void MenuActiveEffects::logic() {
	for(size_t i = 0; i < effect_icons.size(); ++i){
		if(effect_icons[i].stacksLabel){
			delete effect_icons[i].stacksLabel;
		}
	}

	effect_icons.clear();

	for (size_t i = 0; i < stats->effects.effect_list.size(); ++i) {
		if (stats->effects.effect_list[i].icon == -1)
			continue;

		const Effect &ed = stats->effects.effect_list[i];

		size_t most_recent_id = effect_icons.size()-1;
		if(ed.group_stack){
			if( effect_icons.size()>0
				&& effect_icons[most_recent_id].type == ed.type
				&& effect_icons[most_recent_id].name == ed.name){

				effect_icons[most_recent_id].stacks++;

				if(ed.type == Effect::SHIELD){
					//Shields stacks in momment of addition, we never have to reach that
				}else if (ed.type == Effect::HEAL){
					//No special behavior
				}else{
					if(ed.ticks < effect_icons[most_recent_id].current){
						if (ed.duration > 0)
							effect_icons[most_recent_id].overlay.y = (ICON_SIZE * ed.ticks) / ed.duration;
						else
							effect_icons[most_recent_id].overlay.y = ICON_SIZE;
						effect_icons[most_recent_id].current = ed.ticks;
						effect_icons[most_recent_id].max = ed.duration;
					}
				}

				if(!effect_icons[most_recent_id].stacksLabel){
					effect_icons[most_recent_id].stacksLabel = new WidgetLabel();
					effect_icons[most_recent_id].stacksLabel->setX(effect_icons[most_recent_id].pos.x);
					effect_icons[most_recent_id].stacksLabel->setY(effect_icons[most_recent_id].pos.y);
					effect_icons[most_recent_id].stacksLabel->setMaxWidth(ICON_SIZE);
				}

				effect_icons[most_recent_id].stacksLabel->set(msg->get("x%d", effect_icons[most_recent_id].stacks));

				continue;
			}
		}

		EffectIcon ei;
		ei.icon = ed.icon;
		ei.name = ed.name;
		ei.type = ed.type;
		ei.stacks = 1;

		// icon position
		if (!is_vertical) {
			ei.pos.x = window_area.x + (static_cast<int>(effect_icons.size()) * ICON_SIZE);
			ei.pos.y = window_area.y;
		}
		else {
			ei.pos.x = window_area.x;
			ei.pos.y = window_area.y + (static_cast<int>(effect_icons.size()) * ICON_SIZE);
		}
		ei.pos.w = ei.pos.h = ICON_SIZE;

		// timer overlay
		ei.overlay.x = 0;
		ei.overlay.w = ICON_SIZE;

		if (ed.type == Effect::SHIELD) {
			ei.overlay.y = (ICON_SIZE * ed.magnitude) / ed.magnitude_max;
			ei.current = ed.magnitude;
			ei.max = ed.magnitude_max;
		}
		else if (ed.type == Effect::HEAL) {
			ei.overlay.y = ICON_SIZE;
			// current and max are ignored
		}
		else {
			if (ed.duration > 0)
				ei.overlay.y = (ICON_SIZE * ed.ticks) / ed.duration;
			else
				ei.overlay.y = ICON_SIZE;
			ei.current = ed.ticks;
			ei.max = ed.duration;
		}
		ei.overlay.h = ICON_SIZE - ei.overlay.y;

		effect_icons.push_back(ei);
	}

	if (!is_vertical) {
		window_area.w = static_cast<int>(effect_icons.size()) * ICON_SIZE;
		window_area.h = ICON_SIZE;
	}
	else {
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

		if(effect_icons[i].stacksLabel){
			effect_icons[i].stacksLabel->render();
		}
	}
}

TooltipData MenuActiveEffects::checkTooltip(const Point& mouse) {
	TooltipData tip;

	for (size_t i = 0; i < effect_icons.size(); ++i) {
		if (isWithinRect(effect_icons[i].pos, mouse)) {
			if (!effect_icons[i].name.empty())
				tip.addText(msg->get(effect_icons[i].name));

			if (effect_icons[i].type == Effect::HEAL)
				continue;

			std::stringstream ss;
			if (effect_icons[i].type == Effect::SHIELD) {
				ss << "(" << effect_icons[i].current << "/" << effect_icons[i].max << ")";
				tip.addText(ss.str());
			}
			else if (effect_icons[i].max > 0) {
				ss << msg->get("Remaining:") << " " << getDurationString(effect_icons[i].current, 1);
				tip.addText(ss.str());
			}

			if(effect_icons[i].type != Effect::SHIELD){
				if(effect_icons[i].stacks > 1){
					tip.addText(msg->get("x%d stacks", effect_icons[i].stacks));
				}
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
