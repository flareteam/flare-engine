/*
Copyright © 2011-2012 Thane Brimhall
Copyright © 2013 Henrik Andersson
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
 * class CombatText
 *
 * The CombatText class displays floating damage numbers and miss messages
 * above the targets.
 *
 */

#include "CombatText.h"
#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FileParser.h"
#include "FontEngine.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"

#include <stdio.h>

Combat_Text_Item::Combat_Text_Item()
	: label(NULL)
	, lifespan(0)
	, pos(FPoint())
	, floating_offset(0)
	, text("")
	, displaytype(0)
	, is_number(false)
	, number_value(0)
{}

Combat_Text_Item::~Combat_Text_Item() {
	// label deletion is handled by CombatText class
}

CombatText::CombatText() {
	msg_color[MSG_GIVEDMG] = font->getColor(FontEngine::COLOR_COMBAT_GIVEDMG);
	msg_color[MSG_TAKEDMG] = font->getColor(FontEngine::COLOR_COMBAT_TAKEDMG);
	msg_color[MSG_CRIT] = font->getColor(FontEngine::COLOR_COMBAT_CRIT);
	msg_color[MSG_BUFF] = font->getColor(FontEngine::COLOR_COMBAT_BUFF);
	msg_color[MSG_MISS] = font->getColor(FontEngine::COLOR_COMBAT_MISS);

	duration = settings->max_frames_per_sec; // 1 second
	fade_duration = 0;
	speed = Settings::LOGIC_FPS / settings->max_frames_per_sec;
	offset = 48; // average height of flare-game enemies, so a sensible default
	font_id = "font_regular";

	// Load config settings
	FileParser infile;
	// @CLASS CombatText|Description of engine/combat_text.txt
	if(infile.open("engine/combat_text.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if(infile.key == "duration") {
				// @ATTR duration|duration|Duration of the combat text in 'ms' or 's'.
				duration = Parse::toDuration(infile.val);
			}
			else if(infile.key == "speed") {
				// @ATTR speed|float|Motion speed of the combat text.
				speed = (Parse::toFloat(infile.val) * Settings::LOGIC_FPS) / settings->max_frames_per_sec;
			}
			else if (infile.key == "offset") {
				// @ATTR offset|int|The vertical offset for the combat text's starting position.
				offset = Parse::toInt(infile.val);
			}
			else if (infile.key == "fade_duration") {
				// @ATTR fade_duration|duration|How long the combat text will spend fading out in 'ms' or 's'.
				fade_duration = Parse::toDuration(infile.val);
			}
			else if (infile.key == "font") {
				// @ATTR font|predefined_string|The font to use for combat text.
				font_id = infile.val;
			}
			else {
				infile.error("CombatText: '%s' is not a valid key.",infile.key.c_str());
			}
		}
		infile.close();
	}

	if (fade_duration > duration)
		fade_duration = duration;
}

CombatText::~CombatText() {
	// delete all messages
	while (combat_text.size() > 0) {
		delete combat_text.begin()->label;
		combat_text.erase(combat_text.begin());
	}
}

void CombatText::addString(const std::string& message, const FPoint& location, int displaytype) {
	if (!settings->combat_text)
		return;

	Combat_Text_Item c;
	c.pos.x = location.x;
	c.pos.y = location.y;
	c.floating_offset = static_cast<float>(offset);
	c.text = message;
	c.lifespan = duration;
	c.displaytype = displaytype;

	c.label = new WidgetLabel();
	c.label->setPos(static_cast<int>(c.pos.x), static_cast<int>(c.pos.y));
	c.label->setJustify(FontEngine::JUSTIFY_CENTER);
	c.label->setVAlign(LabelInfo::VALIGN_BOTTOM);
	c.label->setFont(font_id);
	c.label->setText(c.text);
	c.label->setColor(msg_color[c.displaytype]);
	combat_text.push_back(c);
}

void CombatText::addFloat(float num, const FPoint& location, int displaytype) {
	if (!settings->combat_text)
		return;

	// when adding multiple combat text of the same type and position on the same frame, add the num to the existing text
	for (std::vector<Combat_Text_Item>::iterator it = combat_text.begin(); it != combat_text.end(); ++it) {
		if (it->is_number && it->displaytype == displaytype && it->lifespan == duration && it->pos.x == location.x && it->pos.y == location.y) {
			it->number_value += num;
			it->text = Utils::floatToString(it->number_value, eset->number_format.combat_text);
			it->label->setText(it->text);
			return;
		}
	}

	addString(Utils::floatToString(num, eset->number_format.combat_text), location, displaytype);

	combat_text.back().is_number = true;
	combat_text.back().number_value = num;
}

void CombatText::logic(const FPoint& _cam) {
	cam = _cam;

	for(std::vector<Combat_Text_Item>::iterator it = combat_text.end(); it != combat_text.begin();) {
		--it;

		it->lifespan--;
		it->floating_offset += speed;

		Point scr_pos;
		scr_pos = Utils::mapToScreen(it->pos.x, it->pos.y, cam.x, cam.y);
		scr_pos.y -= static_cast<int>(it->floating_offset);

		it->label->setPos(scr_pos.x, scr_pos.y);

		// try to prevent messages from overlapping
		for (std::vector<Combat_Text_Item>::iterator overlap_it = it; overlap_it != combat_text.begin();) {
			--overlap_it;
			Rect bounds = *(it->label->getBounds());
			Rect overlap_bounds = *(overlap_it->label->getBounds());
			if (Utils::rectsOverlap(bounds, overlap_bounds)) {
				overlap_it->floating_offset += static_cast<float>(overlap_bounds.h + (overlap_bounds.y - bounds.y));

				scr_pos = Utils::mapToScreen(overlap_it->pos.x, overlap_it->pos.y, cam.x, cam.y);
				scr_pos.y -= static_cast<int>(overlap_it->floating_offset);

				overlap_it->label->setPos(scr_pos.x, scr_pos.y);
			}
		}
	}

	// delete expired messages
	while (combat_text.size() && combat_text.begin()->lifespan <= 0) {
		delete combat_text.begin()->label;
		combat_text.erase(combat_text.begin());
	}
}

void CombatText::render() {
	if (!settings->show_hud) return;

	for(std::vector<Combat_Text_Item>::iterator it = combat_text.begin(); it != combat_text.end(); ++it) {
		if (it->lifespan > 0) {
			// fade out
			if (it->lifespan < fade_duration)
				it->label->setAlpha(static_cast<uint8_t>((static_cast<float>(it->lifespan) / static_cast<float>(fade_duration)) * 255.f));

			it->label->render();
		}
	}
}

void CombatText::clear() {
	while (combat_text.size()) {
		delete combat_text.begin()->label;
		combat_text.erase(combat_text.begin());
	}
}
