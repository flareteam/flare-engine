/*
Copyright © 2011-2012 Thane Brimhall
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

#ifndef COMBAT_TEXT_H
#define COMBAT_TEXT_H

#include "CommonIncludes.h"
#include "Utils.h"

class WidgetLabel;

class Combat_Text_Item {
public:
	Combat_Text_Item();
	~Combat_Text_Item();

	WidgetLabel *label;
	int lifespan;
	FPoint pos;
	float floating_offset;
	std::string text;
	int displaytype;
};

class CombatText {
public:
	CombatText();
	~CombatText();

	void logic(const FPoint& _cam);
	void render();
	void addString(const std::string& message, const FPoint& location, int displaytype);
	void addInt(int num, const FPoint& location, int displaytype);
	void clear();

	enum {
		MSG_GIVEDMG = 0,
		MSG_TAKEDMG = 1,
		MSG_CRIT = 2,
		MSG_MISS = 3,
		MSG_BUFF = 4
	};
private:
	FPoint cam;
	std::vector<Combat_Text_Item> combat_text;

	Color msg_color[5];
	int duration;
	float speed;
	int offset;
};

#endif
