/*
Copyright © 2011-2012 Clint Bellanger
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
 * class MenuStash
 */

#ifndef MENU_STASH_H
#define MENU_STASH_H

#include "CommonIncludes.h"
#include "MenuItemStorage.h"
#include "WidgetLabel.h"

class NPC;
class StatBlock;
class WidgetButton;
class WidgetTabControl;

class MenuStash : public Menu {
private:
	WidgetButton *closeButton;
	WidgetTabControl *tab_control;
	WidgetLabel label_title;
	WidgetLabel label_currency;

	int activetab;
	int drag_prev_tab;

public:
	static const int NO_SLOT = -1;
	static const bool ADD_PLAY_SOUND = true;

	enum {
		STASH_PRIVATE = 0,
		STASH_SHARED = 1
	};

	explicit MenuStash();
	~MenuStash();
	void align();

	void logic();
	void render();
	ItemStack click(const Point& position);
	void itemReturn(ItemStack stack);
	bool add(ItemStack stack, int slot, bool play_sound);
	void renderTooltips(const Point& position);
	bool drop(const Point& position, ItemStack stack);
	void validate(std::queue<ItemStack>& global_drop_stack);

	void removeFromPrevSlot(int quantity);
	void enableSharedTab(bool permadeath);

	void setTab(int tab);
	int getTab() { return activetab; }

	void lockTabControl();
	void unlockTabControl();
	TabList* getCurrentTabList();
	void defocusTabLists();

	Rect slots_area;
	MenuItemStorage stock[2];
	bool updated;

	std::queue<ItemStack> drop_stack;

	TabList tablist_private;
	TabList tablist_shared;
};


#endif
