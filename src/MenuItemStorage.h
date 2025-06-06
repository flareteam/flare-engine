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
 * class MenuItemStorage
 */

#ifndef MENU_ITEM_STORAGE_H
#define MENU_ITEM_STORAGE_H

#include "CommonIncludes.h"
#include "ItemManager.h"
#include "ItemStorage.h"

class StatBlock;
class TooltipData;
class WidgetSlot;

class MenuItemStorage : public ItemStorage {
protected:
	Rect grid_area;
	Point grid_pos;
	int nb_cols;

public:
	MenuItemStorage();
	void initGrid(int _slot_number, const Rect& _area, int nb_cols);
	void initFromList(int _slot_number, const std::vector<Rect>& _area, const std::vector<size_t>& _slot_type);
	~MenuItemStorage();

	// rendering
	void render();
	int slotOver(const Point& position);
	TooltipData checkTooltip(const Point& position, StatBlock *stats, int context, bool input_hint);
	ItemStack click(const Point& position);
	void itemReturn(ItemStack stack);
	void highlightMatching(ItemID item_id);
	void highlightClear();
	void setPos(int x, int y);
	ItemStack getItemStackAtPos(const Point& position);
	std::vector<size_t> slot_type;

	int drag_prev_slot;
	std::vector<WidgetSlot*> slots;
	WidgetSlot *current_slot;
};

#endif


