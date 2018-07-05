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
class WidgetTooltip;

class MenuStash : public Menu {
private:
	StatBlock *stats;
	WidgetButton *closeButton;
	WidgetLabel label_title;
	WidgetLabel label_currency;

	int STASH_SLOTS;

	// label and widget positions
	int slots_cols;
	int slots_rows;

	WidgetTooltip *tip;

public:
	explicit MenuStash(StatBlock *stats);
	~MenuStash();
	void align();

	void logic();
	void render();
	ItemStack click(const Point& position);
	void itemReturn(ItemStack stack);
	bool add(ItemStack stack, int slot, bool play_sound);
	void renderTooltips(const Point& position);
	bool drop(const Point& position, ItemStack stack);

	void removeFromPrevSlot(int quantity);

	Rect slots_area;
	MenuItemStorage stock;
	bool updated;

	std::queue<ItemStack> drop_stack;
};


#endif
