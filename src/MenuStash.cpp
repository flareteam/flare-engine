/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson
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

using namespace std;


MenuStash::MenuStash(StatBlock *_stats)
	: Menu()
	, stats(_stats)
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, color_normal(font->getColor("menu_normal"))
	, stock()
	, updated(false)

{
	background.setGraphics(render_device->loadGraphicSurface("images/menus/stash.png"));

	slots_cols = 8; // default if menus/stash.txt::stash_cols not set
	slots_rows = 8; // default if menus/stash.txt::slots_rows not set

	// Load config settings
	FileParser infile;
	if (infile.open("menus/stash.txt")) {
		while(infile.next()) {
			infile.val = infile.val + ',';

			if (infile.key == "close") {
				close_pos.x = eatFirstInt(infile.val,',');
				close_pos.y = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "slots_area") {
				slots_area.x = eatFirstInt(infile.val,',');
				slots_area.y = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "stash_cols") {
				slots_cols = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "stash_rows") {
				slots_rows = eatFirstInt(infile.val,',');
			}
			else if (infile.key == "label_title") {
				title =  eatLabelInfo(infile.val);
			}
			else if (infile.key == "currency") {
				currency =  eatLabelInfo(infile.val);
			}
		}
		infile.close();
	}

	STASH_SLOTS = slots_cols * slots_rows;
}

void MenuStash::update() {
	slots_area.x += window_area.x;
	slots_area.y += window_area.y;
	slots_area.w = slots_cols*ICON_SIZE;
	slots_area.h = slots_rows*ICON_SIZE;

	stock.init( STASH_SLOTS, slots_area, ICON_SIZE, slots_cols);

	closeButton->pos.x = window_area.x+close_pos.x;
	closeButton->pos.y = window_area.y+close_pos.y;

	for (int i = 0; i < STASH_SLOTS; i++) {
		tablist.add(stock.slots[i]);
	}
}

void MenuStash::logic() {
	if (!visible) return;

	if (NO_MOUSE) {
		tablist.logic();
	}
	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close);
	}
}

void MenuStash::render() {
	if (!visible) return;

	// background
	render_device->render(background);

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

	if (slot != drag_prev_slot) {
		if (stock[slot].item == stack.item) {
			// Merge the stacks
			add(stack, slot);
		}
		else if (stock[slot].item == 0) {
			// Drop the stack
			stock[slot] = stack;
		}
		else if (stock[drag_prev_slot].item == 0) { // Check if the previous slot is free (could still be used if SHIFT was used).
			// Swap the two stacks
			itemReturn(stock[slot]);
			stock[slot] = stack;
		}
		else {
			itemReturn( stack);
		}
	}
	else {
		itemReturn(stack); // cancel
	}

}

void MenuStash::add(ItemStack stack, int slot) {

	if (stack.item != 0) {
		int max_quantity = items->items[stack.item].max_quantity;
		if (slot > -1 && stock[slot].item != 0 && stock[slot].item != stack.item) {
			// the proposed slot isn't available, search for another one
			slot = -1;
		}
		// first search of stack to complete if the item is stackable
		int i = 0;
		while (max_quantity > 1 && slot == -1 && i < STASH_SLOTS) {
			if (stock[i].item == stack.item && stock[i].quantity < max_quantity) {
				slot = i;
			}
			i++;
		}
		// then an empty slot
		i = 0;
		while (slot == -1 && i < STASH_SLOTS) {
			if (stock[i].item == 0) {
				slot = i;
			}
			i++;
		}
		if (slot != -1) {
			// Add
			int quantity_added = min( stack.quantity, max_quantity - stock[slot].quantity);
			stock[slot].item = stack.item;
			stock[slot].quantity += quantity_added;
			stack.quantity -= quantity_added;
			// Add back the remaining
			if (stack.quantity > 0) {
				add( stack);
			}
		}
		else {
			// No available slot, drop
		}
	}
}

/**
 * Start dragging a vendor item
 * Players can drag an item to their inventory.
 */
ItemStack MenuStash::click(Point position) {
	ItemStack stack = stock.click(position);
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

MenuStash::~MenuStash() {
	delete closeButton;
}

