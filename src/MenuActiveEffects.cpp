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
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "TooltipManager.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"
#include <sstream>

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
			// @ATTR wrap_before|bool|Determines where to place icons when they need to wrap to fit on the screen. The default is false, which will place icons from top to bottom (or left to right if using the vertical orientation).
			else if (infile.key == "wrap_before") {
				wrap_before = Parse::toBool(infile.val);
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

	size_t icons_per_row = 1;
	if (!is_vertical) {
		icons_per_row = (settings->view_w - window_area.x) / eset->resolutions.icon_size;
	}
	else {
		icons_per_row = (settings->view_h - window_area.y) / eset->resolutions.icon_size;
	}
	if (icons_per_row < 1) {
		icons_per_row = 1;
	}
	int wrap_dir = (wrap_before ? -1 : 1);

	for (size_t i = 0; i < pc->stats.effects.effect_list.size(); ++i) {
		Effect &ed = pc->stats.effects.effect_list[i];

		if (ed.icon == -1)
			continue;

		size_t most_recent_id = effect_icons.size()-1;

		if (ed.group_stack && effect_icons.size() > 0) {
			EffectIcon& eicon = effect_icons[most_recent_id];
			if (eicon.type == ed.type && eicon.name == ed.name && eicon.icon == ed.icon) {

				eicon.stacks++;

				eicon.timer_current = ed.timer.getCurrent();
				eicon.timer_max = ed.timer.getDuration();

				if (ed.type == Effect::SHIELD){
					//Shields stacks in momment of addition, we never have to reach that
				} else if (ed.type == Effect::HEAL){
					//No special behavior
				} else{
					if (ed.timer.getCurrent() < static_cast<unsigned>(eicon.current)){
						if (ed.timer.getDuration() > 0)
							eicon.overlay.y = (eset->resolutions.icon_size * ed.timer.getCurrent()) / ed.timer.getDuration();
						else
							eicon.overlay.y = eset->resolutions.icon_size;
						eicon.current = eicon.timer_current;
						eicon.max = eicon.timer_max;
					}
				}

				if (!eicon.stacksLabel){
					eicon.stacksLabel = new WidgetLabel();
					eicon.stacksLabel->setPos(eicon.pos.x, eicon.pos.y);
					eicon.stacksLabel->setMaxWidth(eset->resolutions.icon_size);
				}

				std::stringstream ss;
				ss << "×" << eicon.stacks;
				eicon.stacksLabel->setText(ss.str());

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
			ei.pos.x = window_area.x + (static_cast<int>(effect_icons.size() % icons_per_row) * eset->resolutions.icon_size);
			ei.pos.y = window_area.y + (wrap_dir * (static_cast<int>(effect_icons.size() / icons_per_row) * eset->resolutions.icon_size));
		}
		else {
			ei.pos.x = window_area.x + (wrap_dir * (static_cast<int>(effect_icons.size() / icons_per_row) * eset->resolutions.icon_size));
			ei.pos.y = window_area.y + (static_cast<int>(effect_icons.size() % icons_per_row) * eset->resolutions.icon_size);
		}
		ei.pos.w = ei.pos.h = eset->resolutions.icon_size;

		// timer overlay
		ei.overlay.x = 0;
		ei.overlay.w = eset->resolutions.icon_size;

		ei.timer_current = ed.timer.getCurrent();
		ei.timer_max = ed.timer.getDuration();

		if (ed.type == Effect::SHIELD) {
			ei.overlay.y = static_cast<int>((eset->resolutions.icon_size * ed.magnitude) / ed.magnitude_max);
			ei.current = static_cast<int>(ed.magnitude);
			ei.max = static_cast<int>(ed.magnitude_max);
		}
		else if (ed.type == Effect::HEAL) {
			ei.overlay.y = eset->resolutions.icon_size;
			// current and max are ignored
		}
		else {
			if (ed.timer.getDuration() > 0)
				ei.overlay.y = (eset->resolutions.icon_size * ed.timer.getCurrent()) / ed.timer.getDuration();
			else
				ei.overlay.y = eset->resolutions.icon_size;
			ei.current = ei.timer_current;
			ei.max = ei.timer_max;
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
			std::stringstream ss;
			if (!effect_icons[i].name.empty()) {
				ss << msg->get(effect_icons[i].name);
				if (effect_icons[i].type != Effect::SHIELD && effect_icons[i].stacks > 1) {
					ss << " " << "(×" << effect_icons[i].stacks << ")";

				}
				tip_data.addText(ss.str());
			}

			if (effect_icons[i].type == Effect::HEAL)
				continue;

			if (effect_icons[i].type == Effect::SHIELD) {
				ss.str("");
				ss << "(" << effect_icons[i].current << "/" << effect_icons[i].max << ")";
				tip_data.addText(ss.str());
			}
			if (effect_icons[i].max > 0 && effect_icons[i].timer_max > 0) {
				ss.str("");
				ss << msg->get("Remaining:") << " " << Utils::getDurationString(effect_icons[i].timer_current, eset->number_format.durations);
				tip_data.addText(ss.str());
			}

			break;
		}
	}

	tooltipm->push(tip_data, position, TooltipData::STYLE_FLOAT);
}

MenuActiveEffects::~MenuActiveEffects() {
	delete timer;
}
