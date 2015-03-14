/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2014 Henrik Andersson

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
 * class MenuVendor
 */

#ifndef MENU_VENDOR_H
#define MENU_VENDOR_H

#include "CommonIncludes.h"
#include "MenuItemStorage.h"
#include "WidgetLabel.h"

class ItemStorage;
class NPC;
class StatBlock;
class WidgetButton;
class WidgetTabControl;

class MenuVendor : public Menu {
private:
	StatBlock *stats;
	WidgetButton *closeButton;
	WidgetTabControl *tabControl;
	WidgetLabel label_vendor;

	unsigned VENDOR_SLOTS;

	// label and widget positions
	LabelInfo title;
	int slots_cols;
	int slots_rows;
	int activetab;
	Color color_normal;

public:
	MenuVendor(StatBlock *stats);
	~MenuVendor();
	void align();

	NPC *npc;
	ItemStorage buyback_stock;
	MenuItemStorage stock[2]; // items the vendor currently has in stock

	void logic();
	void tabsLogic();
	void setTab(int tab);
	int getTab() {
		return activetab;
	}
	void render();
	ItemStack click(Point position);
	void itemReturn(ItemStack stack);
	void add(ItemStack stack);
	TooltipData checkTooltip(Point position);
	void setInventory();
	void saveInventory();
	void sort(int type);
	int getRowsCount();
	void setNPC(NPC* _npc);

	Rect slots_area;

	TabList tablist;
};


#endif
