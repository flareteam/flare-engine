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

#include "Avatar.h"
#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "IconManager.h"
#include "MenuActiveEffects.h"
#include "Menu.h"
#include "ModManager.h"
#include "MessageEngine.h"
#include "RenderDevice.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "TooltipManager.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"

MenuActiveEffects::MenuActiveEffects()
	: timer(NULL)
	, is_vertical(false)
{
	// Load config settings
	FileParser infile;
	// @CLASS MenuActiveEffects|Description of menus/activeeffects.txt
	if(infile.open("menus/activeeffects.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR vertical|bool|True is vertical orientation; False is horizontal orientation.
			if(infile.key == "vertical") {
				is_vertical = Parse::toBool(infile.val);
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

	for (size_t i = 0; i < pc->stats.effects.effect_list.size(); ++i) {
		if (pc->stats.effects.effect_list[i].icon == -1)
			continue;

		const Effect &ed = pc->stats.effects.effect_list[i];

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
							effect_icons[most_recent_id].overlay.y = (eset->resolutions.icon_size * ed.ticks) / ed.duration;
						else
							effect_icons[most_recent_id].overlay.y = eset->resolutions.icon_size;
						effect_icons[most_recent_id].current = ed.ticks;
						effect_icons[most_recent_id].max = ed.duration;
					}
				}

				if(!effect_icons[most_recent_id].stacksLabel){
					effect_icons[most_recent_id].stacksLabel = new WidgetLabel();
					effect_icons[most_recent_id].stacksLabel->setPos(effect_icons[most_recent_id].pos.x, effect_icons[most_recent_id].pos.y);
					effect_icons[most_recent_id].stacksLabel->setMaxWidth(eset->resolutions.icon_size);
				}

				effect_icons[most_recent_id].stacksLabel->setText(msg->get("x%d", effect_icons[most_recent_id].stacks));

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
			ei.pos.x = window_area.x + (static_cast<int>(effect_icons.size()) * eset->resolutions.icon_size);
			ei.pos.y = window_area.y;
		}
		else {
			ei.pos.x = window_area.x;
			ei.pos.y = window_area.y + (static_cast<int>(effect_icons.size()) * eset->resolutions.icon_size);
		}
		ei.pos.w = ei.pos.h = eset->resolutions.icon_size;

		// timer overlay
		ei.overlay.x = 0;
		ei.overlay.w = eset->resolutions.icon_size;

		if (ed.type == Effect::SHIELD) {
			ei.overlay.y = (eset->resolutions.icon_size * ed.magnitude) / ed.magnitude_max;
			ei.current = ed.magnitude;
			ei.max = ed.magnitude_max;
		}
		else if (ed.type == Effect::HEAL) {
			ei.overlay.y = eset->resolutions.icon_size;
			// current and max are ignored
		}
		else {
			if (ed.duration > 0)
				ei.overlay.y = (eset->resolutions.icon_size * ed.ticks) / ed.duration;
			else
				ei.overlay.y = eset->resolutions.icon_size;
			ei.current = ed.ticks;
			ei.max = ed.duration;
		}
		ei.overlay.h = eset->resolutions.icon_size - ei.overlay.y;

		effect_icons.push_back(ei);
	}

	if (!is_vertical) {
		window_area.w = static_cast<int>(effect_icons.size()) * eset->resolutions.icon_size;
		window_area.h = eset->resolutions.icon_size;
	}
	else {
		window_area.w = eset->resolutions.icon_size;
		window_area.h = static_cast<int>(effect_icons.size()) * eset->resolutions.icon_size;
	}
	Menu::align();
}

void MenuActiveEffects::render() {
	for (size_t i = 0; i < effect_icons.size(); ++i) {
		Point icon_pos(effect_icons[i].pos.x, effect_icons[i].pos.y);
		icons->setIcon(effect_icons[i].icon, icon_pos);
		icons->render();

		if (timer) {
			timer->setClipFromRect(effect_icons[i].overlay);
			timer->setDestFromRect(effect_icons[i].pos);
			render_device->render(timer);
		}

		if(effect_icons[i].stacksLabel){
			effect_icons[i].stacksLabel->render();
		}
	}
}

void MenuActiveEffects::renderTooltips(const Point& position) {
	TooltipData tip_data;

	for (size_t i = 0; i < effect_icons.size(); ++i) {
		if (Utils::isWithinRect(effect_icons[i].pos, position)) {
			if (!effect_icons[i].name.empty())
				tip_data.addText(msg->get(effect_icons[i].name));

			if (effect_icons[i].type == Effect::HEAL)
				continue;

			std::stringstream ss;
			if (effect_icons[i].type == Effect::SHIELD) {
				ss << "(" << effect_icons[i].current << "/" << effect_icons[i].max << ")";
				tip_data.addText(ss.str());
			}
			else if (effect_icons[i].max > 0) {
				ss << msg->get("Remaining:") << " " << Utils::getDurationString(effect_icons[i].current, 1);
				tip_data.addText(ss.str());
			}

			if(effect_icons[i].type != Effect::SHIELD){
				if(effect_icons[i].stacks > 1){
					tip_data.addText(msg->get("x%d stacks", effect_icons[i].stacks));
				}
			}

			break;
		}
	}

	tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
}

MenuActiveEffects::~MenuActiveEffects() {
	delete timer;
}
