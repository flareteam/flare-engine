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

const int ACTIONBAR_MAIN = 10;

class MenuActionBar : public Menu {
private:
	void renderCooldowns();
	FPoint setTarget(bool have_aim, bool aim_assist);

	Sprite *emptyslot;
	Sprite *disabled;
	Sprite *attention;

	Avatar *hero;
	Rect src;

	std::vector<std::string> labels;
	std::vector<std::string> menu_labels;

	Point last_mouse;
	void addSlot(unsigned index, int x, int y);

public:

	MenuActionBar(Avatar *hero);
	~MenuActionBar();
	void align();
	void loadGraphics();
	void renderAttention(int menu_id);
	void logic();
	void render();
	void checkAction(std::vector<ActionData> &action_queue);
	int checkDrag(const Point& mouse);
	void checkMenu(bool &menu_c, bool &menu_i, bool &menu_p, bool &menu_l);
	void drop(const Point& mouse, int power_index, bool rearranging);
	void actionReturn(int power_index);
	void remove(const Point& mouse);
	void set(std::vector<int> power_id);
	void clear();
	void resetSlots();
	void setItemCount(unsigned index, int count, bool is_equipped = false);
	Point getSlotPos(int slot);

	TooltipData checkTooltip(const Point& mouse);
	bool isWithinSlots(const Point& mouse);
	bool isWithinMenus(const Point& mouse);
	void addPower(const int id, const int target_id = 0);

	unsigned slots_count;
	std::vector<int> hotkeys; // refer to power_index in PowerManager
	std::vector<int> hotkeys_temp; // temp for shapeshifting
	std::vector<int> hotkeys_mod; // hotkeys can be changed by items
	std::vector<bool> locked; // if slot is locked, you cannot drop it
	std::vector<WidgetSlot *> slots; // hotkey slots
	WidgetSlot *menus[4]; // menu buttons
	std::vector<int> slot_item_count; // -1 means this power isn't item based.  0 means out of items.  1+ means sufficient items.
	std::vector<bool> slot_enabled;
	bool requires_attention[4];
	std::vector<bool> slot_activated;

	int drag_prev_slot;
	bool updated;
	int twostep_slot;

	TabList tablist;

};

#endif
