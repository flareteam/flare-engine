/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Kurt Rinnert
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
 * class MenuInventory
 */

#ifndef MENU_INVENTORY_H
#define MENU_INVENTORY_H

#include "CommonIncludes.h"
#include "MenuItemStorage.h"
#include "WidgetLabel.h"

class StatBlock;
class WidgetButton;

const int INV_WINDOW = -1;
const int EQUIPMENT = 0;
const int CARRIED = 1;

enum {
	INV_CTRL_NONE = 0,
	INV_CTRL_VENDOR = 1,
	INV_CTRL_STASH = 2
};

class MenuInventory : public Menu {
private:
	StatBlock *stats;

	void loadGraphics();
	void updateEquipment(int slot);

	WidgetLabel label_inventory;
	WidgetLabel label_currency;
	WidgetButton *closeButton;

	int MAX_EQUIPPED;
	int MAX_CARRIED;

	// label and widget positions
	LabelInfo title;
	LabelInfo currency_lbl;
	Rect help_pos;
	int carried_cols;
	int carried_rows;
	Color color_normal;
	Color color_high;
	std::vector<Point> equipped_pos;
	Point carried_pos;

public:
	MenuInventory(StatBlock *stats);
	~MenuInventory();
	void align();

	void logic();
	void render();
	TooltipData checkTooltip(const Point& position);
	int areaOver(const Point& position);

	ItemStack click(const Point& position);
	void itemReturn(ItemStack stack);
	void drop(const Point& position, ItemStack stack);
	void activate(const Point& position);

	void add( ItemStack stack, int area = CARRIED, int slot = -1, bool play_sound = true);
	void remove(int item);
	void removeEquipped(int item);
	void removeFromPrevSlot(int quantity);
	void addCurrency(int count);
	void removeCurrency(int count);
	int getCurrency();
	bool buy(ItemStack stack, int tab);
	bool sell(ItemStack stack);
	bool stashAdd(ItemStack stack);

	bool full(ItemStack stack);
	bool full(int item);
	int getItemCountCarried(int item);
	bool isItemEquipped(int item);
	bool requirementsMet(int item);

	void applyEquipment(ItemStack *equipped);
	void applyItemStats(ItemStack *equipped);
	void applyItemSetBonuses(ItemStack *equipped);
	void applyBonus(const BonusData* bdata);

	int getEquippedCount();
	int getCarriedRows();

	void clearHighlight();

	void fillEquipmentSlots();

	int getMaxPurchasable(int item, int vendor_tab);

	Rect carried_area;
	std::vector<Rect> equipped_area;
	std::vector<std::string> slot_type;
	std::vector<std::string> slot_desc;

	MenuItemStorage inventory[2];
	int currency;
	int drag_prev_src;

	bool changed_equipment;

	short inv_ctrl;

	std::string log_msg;
	std::string show_book;

	std::queue<ItemStack> drop_stack;
};

#endif

