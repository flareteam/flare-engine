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
#include "FileParser.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

Combat_Text_Item::Combat_Text_Item()
	: label(NULL)
	, lifespan(0)
	, pos(FPoint())
	, floating_offset(0)
	, text("")
	, displaytype(0)
{}

Combat_Text_Item::~Combat_Text_Item() {
}

CombatText::CombatText() {
	msg_color[COMBAT_MESSAGE_GIVEDMG] = font->getColor("combat_givedmg");
	msg_color[COMBAT_MESSAGE_TAKEDMG] = font->getColor("combat_takedmg");
	msg_color[COMBAT_MESSAGE_CRIT] = font->getColor("combat_crit");
	msg_color[COMBAT_MESSAGE_BUFF] = font->getColor("combat_buff");
	msg_color[COMBAT_MESSAGE_MISS] = font->getColor("combat_miss");

	duration = MAX_FRAMES_PER_SEC; // 1 second
	speed = 60.f / MAX_FRAMES_PER_SEC;
	offset = 48; // average height of flare-game enemies, so a sensible default

	// Load config settings
	FileParser infile;
	// @CLASS CombatText|Description of engine/combat_text.txt
	if(infile.open("engine/combat_text.txt")) {
		while(infile.next()) {
			if(infile.key == "duration") {
				// @ATTR duration|duration|Duration of the combat text in 'ms' or 's'.
				duration = parse_duration(infile.val);
			}
			else if(infile.key == "speed") {
				// @ATTR speed|int|Motion speed of the combat text.
				speed = static_cast<float>(toInt(infile.val) * 60) / MAX_FRAMES_PER_SEC;
			}
			else if (infile.key == "offset") {
				// @ATTR offset|int|The vertical offset for the combat text's starting position.
				offset = toInt(infile.val);
			}
			else {
				infile.error("CombatText: '%s' is not a valid key.",infile.key.c_str());
			}
		}
		infile.close();
	}
}

CombatText::~CombatText() {
	// delete all messages
	while (combat_text.size() > 0) {
		delete combat_text.begin()->label;
		combat_text.erase(combat_text.begin());
	}
}

void CombatText::addString(const std::string& message, const FPoint& location, int displaytype) {
	if (COMBAT_TEXT) {
		Combat_Text_Item *c = new Combat_Text_Item();
		WidgetLabel *label = new WidgetLabel();
		c->pos.x = location.x;
		c->pos.y = location.y;
		c->floating_offset = static_cast<float>(offset);
		c->label = label;
		c->text = message;
		c->lifespan = duration;
		c->displaytype = displaytype;

		c->label->set(static_cast<int>(c->pos.x), static_cast<int>(c->pos.y), JUSTIFY_CENTER, VALIGN_BOTTOM, c->text, msg_color[c->displaytype]);
		combat_text.push_back(*c);
		delete c;
	}
}

void CombatText::addInt(int num, const FPoint& location, int displaytype) {
	if (COMBAT_TEXT) {
		std::stringstream ss;
		ss << num;
		addString(ss.str(), location, displaytype);
	}
}

void CombatText::logic(const FPoint& _cam) {
	cam = _cam;

	for(std::vector<Combat_Text_Item>::iterator it = combat_text.begin(); it != combat_text.end(); ++it) {
		it->lifespan--;
		it->floating_offset += speed;

		Point scr_pos;
		scr_pos = map_to_screen(it->pos.x, it->pos.y, cam.x, cam.y);
		scr_pos.y -= static_cast<int>(it->floating_offset);

		it->label->setX(scr_pos.x);
		it->label->setY(scr_pos.y);
	}

	// delete expired messages
	while (combat_text.size() && combat_text.begin()->lifespan <= 0) {
		delete combat_text.begin()->label;
		combat_text.erase(combat_text.begin());
	}
}

void CombatText::render() {
	if (!SHOW_HUD) return;

	for(std::vector<Combat_Text_Item>::iterator it = combat_text.begin(); it != combat_text.end(); ++it) {
		if (it->lifespan > 0)
			it->label->render();
	}
}

void CombatText::clear() {
	combat_text.clear();
}
