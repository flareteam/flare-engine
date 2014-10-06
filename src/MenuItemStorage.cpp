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
 * class MenuItemStorage
 */

#include "MenuItemStorage.h"
#include "Settings.h"
#include "SharedResources.h"
#include "SharedGameResources.h"

using namespace std;

MenuItemStorage::MenuItemStorage()
	: grid_area()
	, nb_cols(0)
	, slot_type()
	, drag_prev_slot(-1)
	, slots()
	, highlight(NULL)
	, highlight_image(NULL)
	, overlay_disabled(NULL) {
}

void MenuItemStorage::init(int _slot_number, Rect _area, int _icon_size, int _nb_cols) {
	ItemStorage::init( _slot_number);
	grid_area = _area;
	for (int i = 0; i < _slot_number; i++) {
		WidgetSlot *slot = new WidgetSlot();
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
void MenuItemStorage::init(int _slot_number, vector<Rect> _area, vector<string> _slot_type) {
	ItemStorage::init( _slot_number);
	for (int i = 0; i < _slot_number; i++) {
		WidgetSlot *slot = new WidgetSlot();
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
	Image *graphics;
	graphics = render_device->loadImage("images/menus/attention_glow.png",
			   "Couldn't load icon highlight image");
	if (graphics) {
		highlight_image = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/disabled.png");
	if (graphics) {
		overlay_disabled = graphics->createSprite();
		graphics->unref();
	}

}

void MenuItemStorage::render() {
	Rect disabled_src;
	disabled_src.x = disabled_src.y = 0;
	disabled_src.w = disabled_src.h = ICON_SIZE;

	for (int i=0; i<slot_number; i++) {
		if (storage[i].item > 0) {
			slots[i]->setIcon(items->items[storage[i].item].icon);
			slots[i]->setAmount(storage[i].quantity, items->items[storage[i].item].max_quantity);
		}
		else {
			slots[i]->setIcon(-1);
		}
		slots[i]->render();
		if (!slots[i]->enabled) {
			if (overlay_disabled) {
				overlay_disabled->setClip(disabled_src);
				overlay_disabled->setDest(slots[i]->pos);
				render_device->render(overlay_disabled);
			}
		}
		if (highlight[i]) renderHighlight(slots[i]->pos.x, slots[i]->pos.y, slots[i]->pos.w);
	}
}

void MenuItemStorage::renderHighlight(int x, int y, int _icon_size) {
	if (_icon_size == ICON_SIZE) {
		Rect dest;
		dest.x = x;
		dest.y = y;
		if (highlight_image) {
			highlight_image->setDest(dest);
			render_device->render(highlight_image);
		}
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

	// try to click on the highlighted (aka in focus) slot
	// since mouse clicks defocus slots before this point,
	// we don't have to worry about the mouse being over another slot
	if (drag_prev_slot == -1) {
		for (unsigned int i=0; i<slots.size(); i++) {
			if (slots[i]->in_focus) {
				drag_prev_slot = i;
				break;
			}
		}
	}

	if (drag_prev_slot > -1) {
		item = storage[drag_prev_slot];
		if (inpt->pressing[SHIFT] || NO_MOUSE || inpt->touch_locked) {
			item.quantity = 1;
		}
		substract( drag_prev_slot, item.quantity);
		return item;
	}
	else {
		item.clear();
		return item;
	}
}

void MenuItemStorage::itemReturn(ItemStack stack) {
	add( stack, drag_prev_slot);
	drag_prev_slot = -1;
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
	if (highlight_image)
		delete highlight_image;

	delete[] highlight;

	if (overlay_disabled)
		delete overlay_disabled;

	for (unsigned i=0; i<slots.size(); i++)
		delete slots[i];
}
