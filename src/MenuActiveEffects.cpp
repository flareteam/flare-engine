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
	graphics = render_device->loadImage("images/menus/disabled.png");
	if (graphics) {
		timer = graphics->createSprite();
		graphics->unref();
	}
}

void MenuActiveEffects::logic() {
	for(size_t i; i < effect_icons.size(); ++i){
		if(effect_icons[i].stacksLabel){
			delete effect_icons[i].stacksLabel;
		}
	}

	effect_icons.clear();

	bool alreadyOn;

	for (size_t i = 0; i < stats->effects.effect_list.size(); ++i) {
		if (stats->effects.effect_list[i].icon == -1)
			continue;

		const Effect &ed = stats->effects.effect_list[i];
		
		if(ed.group_stack){
			alreadyOn = false;
			for(size_t j=0; j < effect_icons.size(); ++j){
				if(effect_icons[j].type == ed.type && effect_icons[j].name == ed.name){
					effect_icons[j].stacks++;

					if(ed.type == EFFECT_SHIELD){
						if(ed.magnitude < effect_icons[j].overlay.y/ICON_SIZE*effect_icons[j].max){
							effect_icons[j].overlay.y = (ICON_SIZE * ed.magnitude)/ ed.magnitude_max;
						}
						effect_icons[j].current += ed.magnitude;
						effect_icons[j].max += ed.magnitude_max;
					}else if (ed.type == EFFECT_HEAL){
						//No special behavior
					}else{
						if(ed.ticks < effect_icons[j].current){
							if (ed.duration > 0)
								effect_icons[j].overlay.y = (ICON_SIZE * ed.ticks) / ed.duration;
							else
								effect_icons[j].overlay.y = ICON_SIZE;
							effect_icons[j].current = ed.ticks;
							effect_icons[j].max = ed.duration;
						}
					}

					if(!effect_icons[j].stacksLabel){
						effect_icons[j].stacksLabel = new WidgetLabel();

						effect_icons[j].stacksLabel->setX(effect_icons[j].pos.x);
						effect_icons[j].stacksLabel->setY(effect_icons[j].pos.y);
						effect_icons[j].stacksLabel->setMaxWidth(ICON_SIZE);
					}

					std::stringstream ss;
					
					ss << msg->get("x%d", effect_icons[j].stacks);
					effect_icons[j].stacksLabel->set(ss.str());	

					alreadyOn = true;
					break;
				}
			}
			
			if(alreadyOn){
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

			if (effect_icons[i].type == EFFECT_HEAL)
				continue;

			std::stringstream ss;
			if (effect_icons[i].type == EFFECT_SHIELD) {
				ss << "(" << effect_icons[i].current << "/" << effect_icons[i].max << ")";
				tip.addText(ss.str());
			}
			else if (effect_icons[i].max > 0) {
				ss << msg->get("Remaining:") << " " << getDurationString(effect_icons[i].current, 1);
				tip.addText(ss.str());
			}

			if(effect_icons[i].type != EFFECT_SHIELD){
				std::stringstream ss2;
				if(effect_icons[i].stacks > 1){
					ss2 << msg->get("x%d stacks", effect_icons[i].stacks);;
					tip.addText(ss2.str());
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
