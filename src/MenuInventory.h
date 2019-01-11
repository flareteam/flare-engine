/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
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
 * class MenuInventory
 */

#ifndef MENU_INVENTORY_H
#define MENU_INVENTORY_H

#include "CommonIncludes.h"
#include "MenuItemStorage.h"
#include "Utils.h"
#include "WidgetLabel.h"

class BonusData;
class StatBlock;
class WidgetButton;

class MenuInventory : public Menu {
private:
	static const bool ONLY_EMPTY_SLOTS = true;

	void loadGraphics();
	void updateEquipment(int slot);
	int getEquipSlotFromItem(int item, bool only_empty_slots);

	WidgetLabel label_inventory;
	WidgetLabel label_currency;
	WidgetButton *closeButton;

	int MAX_EQUIPPED;
	int MAX_CARRIED;

	// label and widget positions
	Rect help_pos;
	int carried_cols;
	int carried_rows;
	std::vector<Point> equipped_pos;
	Point carried_pos;

	Timer tap_to_activate_timer;

	int activated_slot;
	int activated_item;

public:
	enum {
		CTRL_NONE = 0,
		CTRL_VENDOR = 1,
		CTRL_STASH = 2
	};

	static const int NO_AREA = -1;
	static const int EQUIPMENT = 0;
	static const int CARRIED = 1;

	static const bool ADD_PLAY_SOUND = true;
	static const bool ADD_AUTO_EQUIP = true;
	static const bool IS_DRAGGING = true;

	explicit MenuInventory();
	~MenuInventory();
	void align();

	void logic();
	void render();
	void renderTooltips(const Point& position);
	int areaOver(const Point& position);

	ItemStack click(const Point& position);
	void itemReturn(ItemStack stack);
	bool drop(const Point& position, ItemStack stack);
	void activate(const Point& position);

	bool add(ItemStack stack, int area, int slot, bool play_sound, bool auto_equip);
	bool remove(int item);
	void removeFromPrevSlot(int quantity);
	void addCurrency(int count);
	void removeCurrency(int count);
	bool buy(ItemStack stack, int tab, bool dragging);
	bool sell(ItemStack stack);

	bool requirementsMet(int item);

	void applyEquipment();
	void applyItemStats();
	void applyItemSetBonuses();
	void applyBonus(const BonusData* bdata);

	int getEquippedCount();
	int getTotalSlotCount();

	void clearHighlight();

	void fillEquipmentSlots();

	int getMaxPurchasable(ItemStack item, int vendor_tab);

	int getPowerMod(int meta_power);

	Rect carried_area;
	std::vector<Rect> equipped_area;
	std::vector<std::string> slot_type;

	MenuItemStorage inventory[2];
	int currency;
	int drag_prev_src;

	bool changed_equipment;

	short inv_ctrl;

	std::string show_book;

	std::queue<ItemStack> drop_stack;
};

#endif

