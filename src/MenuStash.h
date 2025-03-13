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

class MenuStashTab {
public:
	static const bool IS_PRIVATE = true;

	MenuStashTab(const std::string& id, const std::string& _name, const std::string& _filename, bool _is_private);
	~MenuStashTab();

	std::string id;
	std::string name;
	std::string filename;
	bool is_private;
	bool is_legacy;
	bool updated;

	MenuItemStorage stock;
	TabList tablist;
};

class MenuStash : public Menu {
private:
	WidgetButton *closeButton;
	WidgetTabControl *tab_control;
	WidgetLabel label_title;
	WidgetLabel label_currency;

	size_t activetab;

public:
	static const int NO_SLOT = -1;
	static const bool ADD_PLAY_SOUND = true;

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
	bool checkUpdates();

	void removeFromPrevSlot(int quantity);
	void enableSharedTab(bool permadeath);

	void setTab(size_t tab);
	size_t getTab();

	void lockTabControl();
	void unlockTabControl();
	TabList* getCurrentTabList();
	void defocusTabLists();

	Rect slots_area;
	std::vector<MenuStashTab> tabs;

	std::queue<ItemStack> drop_stack;

	bool lock_tab_control;
};


#endif
