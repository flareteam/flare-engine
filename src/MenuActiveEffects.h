/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2014 Justin Jacobs
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

#ifndef MENU_ACTIVE_EFFECTS_H
#define MENU_ACTIVE_EFFECTS_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "Utils.h"

class StatBlock;
class TooltipData;
class WidgetLabel;

class EffectIcon {
public:
	int icon;
	int type;
	int current;
	int max;
	int stacks;
	Rect pos;
	Rect overlay;
	std::string name;
	WidgetLabel* stacksLabel;

	EffectIcon()
		: icon(-1)
		, type(0)
		, current(0)
		, max(0)
		, stacks(0)
		, stacksLabel(NULL)
	{}
};

class MenuActiveEffects : public Menu {
private:
	Sprite *timer;
	bool is_vertical;
	std::vector<EffectIcon> effect_icons;

public:
	explicit MenuActiveEffects();
	~MenuActiveEffects();
	void loadGraphics();
	void logic();
	void render();
	void renderTooltips(const Point& position);
};

#endif
