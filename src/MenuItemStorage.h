/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk

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


#pragma once
#ifndef MENU_ITEM_STORAGE_H
#define MENU_ITEM_STORAGE_H

#include "CommonIncludes.h"
#include "ItemManager.h"
#include "ItemStorage.h"
#include "WidgetSlot.h"

class TooltipData;

class MenuItemStorage : public ItemStorage {
protected:
	SDL_Surface *icons;
	void loadGraphics();
	void renderHighlight(int x, int y, int _icon_size);
	SDL_Rect grid_area;
	int nb_cols;

public:
	MenuItemStorage();
	void init(int _slot_number, SDL_Rect _area, int icon_size, int nb_cols);
	void init(int _slot_number, std::vector<SDL_Rect> _area, std::vector<std::string> _slot_type);
	~MenuItemStorage();

	// rendering
	void render();
	int slotOver(Point position);
	TooltipData checkTooltip(Point position, StatBlock *stats, int context);
	ItemStack click(Point position);
	void itemReturn(ItemStack stack);
	void fillEquipmentSlots();
	void highlightMatching(std::string type);
	void highlightClear();
	std::vector<std::string> slot_type;

	int drag_prev_slot;
	std::vector<WidgetSlot*> slots;

	bool * highlight;
	SDL_Surface * highlight_image;
};

#endif


