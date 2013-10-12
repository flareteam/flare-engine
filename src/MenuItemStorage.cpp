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

#include "MenuItemStorage.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SharedGameResources.h"

using namespace std;

MenuItemStorage::MenuItemStorage()
	: icons(NULL)
	, grid_area()
	, nb_cols(0)
	, slot_type()
	, drag_prev_slot(-1)
	, slots()
	, highlight(NULL)
	, highlight_image(NULL) {
}

void MenuItemStorage::init(int _slot_number, SDL_Rect _area, int _icon_size, int _nb_cols) {
	ItemStorage::init( _slot_number);
	icons = items->getIcons();
	grid_area = _area;
	for (int i = 0; i < _slot_number; i++) {
		WidgetSlot *slot = new WidgetSlot(icons);
		slots.push_back(slot);
	}
	nb_cols = _nb_cols;
	highlight = new bool[_slot_number];
	for (int i=0; i<_slot_number; i++) {
		highlight[i] = false;
		slots[i]->pos.x = grid_area.x + (i % nb_cols * _icon_size);
		slots[i]->pos.y = grid_area.y + (i / nb_cols * _icon_size);
		slots[i]->pos.h = slots[i]->pos.w = _icon_size;
	}
	loadGraphics();
}

/**
 * Overloaded function for case, if slot positions are predefined
 */
void MenuItemStorage::init(int _slot_number, vector<SDL_Rect> _area, vector<string> _slot_type) {
	ItemStorage::init( _slot_number);
	icons = items->getIcons();
	for (int i = 0; i < _slot_number; i++) {
		WidgetSlot *slot = new WidgetSlot(icons);
		slot->pos = _area[i];
		slots.push_back(slot);
	}
	nb_cols = 0;
	slot_type = _slot_type;
	highlight = new bool[_slot_number];
	for (int i=0; i<_slot_number; i++) {
		highlight[i] = false;
	}
	loadGraphics();
}

void MenuItemStorage::loadGraphics() {
	highlight_image = loadGraphicSurface("images/menus/attention_glow.png", "Couldn't load icon highlight image");
}

void MenuItemStorage::render() {
	for (int i=0; i<slot_number; i++) {
		if (storage[i].item > 0) {
			slots[i]->setIcon(items->items[storage[i].item].icon);
			slots[i]->setAmount(storage[i].quantity, items->items[storage[i].item].max_quantity);
		}
		else {
			slots[i]->setIcon(-1);
		}
		slots[i]->render();
		if (highlight[i]) renderHighlight(slots[i]->pos.x, slots[i]->pos.y, slots[i]->pos.w);
	}
}

void MenuItemStorage::renderHighlight(int x, int y, int _icon_size) {
	if (_icon_size == ICON_SIZE) {
		SDL_Rect dest;
		dest.x = x;
		dest.y = y;
		SDL_BlitSurface(highlight_image,NULL,screen,&dest);
	}
}

int MenuItemStorage::slotOver(Point position) {
	if (isWithin(grid_area, position) && nb_cols > 0) {
		return (position.x - grid_area.x) / slots[0]->pos.w + (position.y - grid_area.y) / slots[0]->pos.w * nb_cols;
	}
	else if (nb_cols == 0) {
		for (unsigned int i=0; i<slots.size(); i++) {
			if (isWithin(slots[i]->pos, position)) return i;
		}
	}
	return -1;
}

TooltipData MenuItemStorage::checkTooltip(Point position, StatBlock *stats, int context) {
	TooltipData tip;
	int slot = slotOver(position);

	if (slot > -1 && storage[slot].item > 0) {
		return items->getTooltip(storage[slot], stats, context);
	}
	return tip;
}

ItemStack MenuItemStorage::click(Point position) {
	ItemStack item;
	drag_prev_slot = slotOver(position);
	if (drag_prev_slot == -1) {
		// FIXME: What if mouse is over one slot and focused is another slot
		for (unsigned int i=0; i<slots.size(); i++) {
			if (slots[i]->in_focus) {
				drag_prev_slot = i;
				break;
			}
		}
	}

	if (drag_prev_slot > -1) {
		item = storage[drag_prev_slot];
		if (inpt->pressing[SHIFT] || NO_MOUSE) {
			item.quantity = 1;
		}
		substract( drag_prev_slot, item.quantity);
		return item;
	}
	else {
		item.item = 0;
		item.quantity = 0;
		return item;
	}
}

void MenuItemStorage::itemReturn(ItemStack stack) {
	add( stack, drag_prev_slot);
	drag_prev_slot = -1;
}

/**
 * Sort storage array, so items order matches slots order
 */
void MenuItemStorage::fillEquipmentSlots() {
	// create temporary arrays
	int *equip_item = new int[slot_number];
	int *equip_quantity = new int[slot_number];;

	// initialize arrays
	for (int i=0; i<slot_number; i++) {
		equip_item[i] = storage[i].item;
		equip_quantity[i] = storage[i].quantity;
	}
	// clean up storage[]
	for (int i=0; i<slot_number; i++) {
		storage[i].item = 0;
		storage[i].quantity = 0;
	}

	// fill slots with items
	for (int i=0; i<slot_number; i++) {
		for (int j=0; j<slot_number; j++) {
			// search for empty slot with needed type. If item is not NULL, put it there
			if (items->items[equip_item[i]].type == slot_type[j] && equip_item[i] > 0 && storage[j].item == 0) {
				storage[j].item = equip_item[i];
				storage[j].quantity = (equip_quantity[i] > 0) ? equip_quantity[i] : 1;
				break;
			}
		}
	}
	delete [] equip_item;
	delete [] equip_quantity;
}

void MenuItemStorage::highlightMatching(string type) {
	for (int i=0; i<slot_number; i++) {
		if (slot_type[i] == type) highlight[i] = true;
	}
}

void MenuItemStorage::highlightClear() {
	for (int i=0; i<slot_number; i++) {
		highlight[i] = false;
	}
}

MenuItemStorage::~MenuItemStorage() {
	delete[] highlight;
	SDL_FreeSurface(highlight_image);
	for (unsigned i=0; i<slots.size(); i++)
		delete slots[i];
}
