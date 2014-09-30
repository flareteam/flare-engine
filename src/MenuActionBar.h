/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
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
 * class ActionBar
 *
 * Handles the config, display, and usage of the 0-9 hotkeys, mouse buttons, and menu calls
 */

#pragma once
#ifndef MENU_ACTION_BAR_H
#define MENU_ACTION_BAR_H

#include "CommonIncludes.h"
#include "Avatar.h"
#include "Menu.h"
#include "Utils.h"
#include "WidgetLabel.h"

class StatBlock;
class TooltipData;
class WidgetLabel;
class WidgetSlot;

const int MENU_CHARACTER = 0;
const int MENU_INVENTORY = 1;
const int MENU_POWERS = 2;
const int MENU_LOG = 3;

class MenuActionBar : public Menu {
private:
	void alignElements();
	void renderCooldowns();
	FPoint setTarget(bool have_aim, bool aim_assist);

	Sprite *emptyslot;
	Sprite *disabled;
	Sprite *attention;

	Avatar *hero;
	Rect src;

	WidgetLabel *labels[16];
	Point last_mouse;

public:

	MenuActionBar(Avatar *hero);
	~MenuActionBar();
	void loadGraphics();
	void renderAttention(int menu_id);
	void logic();
	void render();
	void checkAction(std::vector<ActionData> &action_queue);
	int checkDrag(Point mouse);
	void checkMenu(bool &menu_c, bool &menu_i, bool &menu_p, bool &menu_l);
	void drop(Point mouse, int power_index, bool rearranging);
	void actionReturn(int power_index);
	void remove(Point mouse);
	void set(int power_id[12]);
	void clear();
	void resetSlots();

	TooltipData checkTooltip(Point mouse);

	int hotkeys[12]; // refer to power_index in PowerManager
	int hotkeys_temp[12]; // temp for shapeshifting
	int hotkeys_mod[12]; // hotkeys can be changed by items
	bool locked[12]; // if slot is locked, you cannot drop it
	WidgetSlot *slots[12]; // hotkey slots
	WidgetSlot *menus[4]; // menu buttons
	int slot_item_count[12]; // -1 means this power isn't item based.  0 means out of items.  1+ means sufficient items.
	bool slot_enabled[12];
	bool requires_attention[4];
	bool slot_activated[12];

	// these store the area occupied by these hotslot sections.
	// useful for detecting mouse interactions on those locations
	Rect numberArea;
	Rect mouseArea;
	Rect menuArea;
	int drag_prev_slot;
	bool updated;
	int twostep_slot;

	TabList tablist;

};

#endif
