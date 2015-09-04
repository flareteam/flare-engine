/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson
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
 * class MenuStash
 */

#include "FileParser.h"
#include "Menu.h"
#include "MenuStash.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "SharedGameResources.h"

MenuStash::MenuStash(StatBlock *_stats)
	: Menu()
	, stats(_stats)
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, color_normal(font->getColor("menu_normal"))
	, stock()
	, updated(false)

{

	setBackground("images/menus/stash.png");

	slots_cols = 8; // default if menus/stash.txt::stash_cols not set
	slots_rows = 8; // default if menus/stash.txt::slots_rows not set

	// Load config settings
	FileParser infile;
	// @CLASS MenuStash|Description of menus/stash.txt
	if (infile.open("menus/stash.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|x (integer), y (integer)|Position of the close button.
			if (infile.key == "close") {
				Point pos = toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y);
			}
			// @ATTR slots_area|x (integer), y (integer)|Position of the top-left slot.
			else if (infile.key == "slots_area") {
				slots_area.x = popFirstInt(infile.val);
				slots_area.y = popFirstInt(infile.val);
			}
			// @ATTR stash_cols|integer|The number of columns for the grid of slots.
			else if (infile.key == "stash_cols") {
				slots_cols = std::max(1, toInt(infile.val));
			}
			// @ATTR stash_rows|integer|The number of rows for the grid of slots.
			else if (infile.key == "stash_rows") {
				slots_rows = std::max(1, toInt(infile.val));
			}
			// @ATTR label_title|label|Position of the "Stash" label.
			else if (infile.key == "label_title") {
				title =  eatLabelInfo(infile.val);
			}
			// @ATTR currency|label|Position of the label displaying the amount of currency stored in the stash.
			else if (infile.key == "currency") {
				currency =  eatLabelInfo(infile.val);
			}
			else {
				infile.error("MenuStash: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	STASH_SLOTS = slots_cols * slots_rows;
	slots_area.w = slots_cols*ICON_SIZE;
	slots_area.h = slots_rows*ICON_SIZE;

	stock.init( STASH_SLOTS, slots_area, ICON_SIZE, slots_cols);
	for (int i = 0; i < STASH_SLOTS; i++) {
		tablist.add(stock.slots[i]);
	}

	align();
}

void MenuStash::align() {
	Menu::align();

	closeButton->setPos(window_area.x, window_area.y);
	stock.setPos(window_area.x, window_area.y);
}

void MenuStash::logic() {
	if (!visible) return;

	tablist.logic();

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close);
	}
}

void MenuStash::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();

	// text overlay
	WidgetLabel label;
	if (!title.hidden) {
		label.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("Shared Stash"), color_normal, title.font_style);
		label.render();
	}
	if (!currency.hidden) {
		label.set(window_area.x+currency.x, window_area.y+currency.y, currency.justify, currency.valign, msg->get("%d %s", stock.count(CURRENCY_ID), CURRENCY), color_normal, currency.font_style);
		label.render();
	}


	// show stock
	stock.render();
}

/**
 * Dragging and dropping an item can be used to rearrange the stash
 */
void MenuStash::drop(Point position, ItemStack stack) {
	int slot;
	int drag_prev_slot;

	items->playSound(stack.item);

	slot = stock.slotOver(position);
	drag_prev_slot = stock.drag_prev_slot;

	if (slot == -1) {
		itemReturn(stack);
	}
	else if (slot != drag_prev_slot) {
		if (stock[slot].item == stack.item || stock[slot].empty()) {
			// Drop the stack, merging if needed
			add(stack, slot);
		}
		else if (drag_prev_slot != -1 && stock[drag_prev_slot].empty()) {
			// Check if the previous slot is free (could still be used if SHIFT was used).
			// Swap the two stacks
			itemReturn(stock[slot]);
			stock[slot] = stack;
		}
		else {
			itemReturn(stack);
		}
	}
	else {
		itemReturn(stack); // cancel
	}

	updated = true;
}

void MenuStash::add(ItemStack stack, int slot) {
	stock.add(stack, slot);
	updated = true;
}

/**
 * Start dragging a vendor item
 * Players can drag an item to their inventory.
 */
ItemStack MenuStash::click(Point position) {
	ItemStack stack = stock.click(position);
	if (TOUCHSCREEN) {
		tablist.setCurrent(stock.current_slot);
	}
	return stack;
}

/**
 * Cancel the dragging initiated by the click()
 */
void MenuStash::itemReturn(ItemStack stack) {
	stock.itemReturn(stack);
}

void MenuStash::add(ItemStack stack) {
	items->playSound(stack.item);
	stock.add(stack);
	updated = true;
}

TooltipData MenuStash::checkTooltip(Point position) {
	return stock.checkTooltip(position, stats, PLAYER_INV);
}

bool MenuStash::full(int item) {
	return stock.full(item);
}

int MenuStash::getRowsCount() {
	return slots_rows;
}

void MenuStash::removeFromPrevSlot(int quantity) {
	int drag_prev_slot = stock.drag_prev_slot;
	if (drag_prev_slot > -1) {
		stock.subtract(drag_prev_slot, quantity);
	}
}

MenuStash::~MenuStash() {
	delete closeButton;
}

