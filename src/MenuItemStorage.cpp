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

#include "EngineSettings.h"
#include "ItemManager.h"
#include "MenuItemStorage.h"
#include "Settings.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "SharedGameResources.h"
#include "TooltipData.h"
#include "WidgetSlot.h"

MenuItemStorage::MenuItemStorage()
	: grid_area()
	, nb_cols(0)
	, slot_type()
	, drag_prev_slot(-1)
	, slots()
	, current_slot(NULL)
	, highlight(NULL)
	, highlight_image(NULL)
	, overlay_disabled(NULL) {
}

void MenuItemStorage::initGrid(int _slot_number, const Rect& _area, int _nb_cols) {
	ItemStorage::init( _slot_number);
	grid_area = _area;
	grid_pos.x = _area.x;
	grid_pos.y = _area.y;
	for (int i = 0; i < _slot_number; i++) {
		WidgetSlot *slot = new WidgetSlot(WidgetSlot::NO_ICON, Input::ACCEPT);
		slots.push_back(slot);
	}
	nb_cols = _nb_cols;
	highlight = new bool[_slot_number];
	for (int i=0; i<_slot_number; i++) {
		highlight[i] = false;
		slots[i]->pos.x = grid_area.x + (i % nb_cols * eset->resolutions.icon_size);
		slots[i]->pos.y = grid_area.y + (i / nb_cols * eset->resolutions.icon_size);
		slots[i]->pos.h = slots[i]->pos.w = eset->resolutions.icon_size;
		slots[i]->setBasePos(slots[i]->pos.x, slots[i]->pos.y, Utils::ALIGN_TOPLEFT);
	}
	loadGraphics();
}

void MenuItemStorage::initFromList(int _slot_number, const std::vector<Rect>& _area, const std::vector<std::string>& _slot_type) {
	ItemStorage::init( _slot_number);
	for (int i = 0; i < _slot_number; i++) {
		WidgetSlot *slot = new WidgetSlot(WidgetSlot::NO_ICON, Input::ACCEPT);
		slot->pos = _area[i];
		slot->setBasePos(slot->pos.x, slot->pos.y, Utils::ALIGN_TOPLEFT);
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

void MenuItemStorage::setPos(int x, int y) {
	for (unsigned i=0; i<slots.size(); ++i) {
		slots[i]->setPos(x, y);
	}
	if (nb_cols > 0) {
		grid_area.x = grid_pos.x + x;
		grid_area.y = grid_pos.y + y;
	}
}

void MenuItemStorage::loadGraphics() {
	Image *graphics;
	graphics = render_device->loadImage("images/menus/attention_glow.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		highlight_image = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/disabled.png", RenderDevice::ERROR_NORMAL);
	if (graphics) {
		overlay_disabled = graphics->createSprite();
		graphics->unref();
	}

}

void MenuItemStorage::render() {
	Rect disabled_src;
	disabled_src.x = disabled_src.y = 0;
	disabled_src.w = disabled_src.h = eset->resolutions.icon_size;

	for (int i=0; i<slot_number; i++) {
		if (storage[i].item > 0) {
			slots[i]->setIcon(items->items[storage[i].item].icon, items->getItemIconOverlay(storage[i].item));
			slots[i]->setAmount(storage[i].quantity, items->items[storage[i].item].max_quantity);
		}
		else {
			slots[i]->setIcon(WidgetSlot::NO_ICON, WidgetSlot::NO_OVERLAY);
		}
		slots[i]->render();
		if (!slots[i]->enabled) {
			if (overlay_disabled) {
				overlay_disabled->setClipFromRect(disabled_src);
				overlay_disabled->setDestFromRect(slots[i]->pos);
				render_device->render(overlay_disabled);
			}
		}
		if (highlight[i] && !slots[i]->in_focus) {
			if (highlight_image) {
				highlight_image->setDestFromRect(slots[i]->pos);
				render_device->render(highlight_image);
			}
		}
	}
}

int MenuItemStorage::slotOver(const Point& position) {
	if (Utils::isWithinRect(grid_area, position) && nb_cols > 0) {
		return (position.x - grid_area.x) / slots[0]->pos.w + (position.y - grid_area.y) / slots[0]->pos.w * nb_cols;
	}
	else if (nb_cols == 0) {
		for (unsigned int i=0; i<slots.size(); i++) {
			if (Utils::isWithinRect(slots[i]->pos, position)) return i;
		}
	}
	return -1;
}

TooltipData MenuItemStorage::checkTooltip(const Point& position, StatBlock *stats, int context) {
	TooltipData tip;
	int slot = slotOver(position);

	if (slot > -1 && storage[slot].item > 0) {
		return items->getTooltip(storage[slot], stats, context);
	}
	return tip;
}

ItemStack MenuItemStorage::click(const Point& position) {
	ItemStack item;

	drag_prev_slot = slotOver(position);

	// no selection, so defocus everything
	if (drag_prev_slot == -1) {
		for (unsigned int i=0; i<slots.size(); i++) {
			if (slots[i]->in_focus) {
				slots[i]->defocus();
			}

			if (slots[i] == current_slot) {
				current_slot = NULL;
			}
		}
	}

	if (drag_prev_slot > -1) {
		item = storage[drag_prev_slot];
		if (settings->touchscreen) {
			if (!slots[drag_prev_slot]->in_focus && !item.empty()) {
				slots[drag_prev_slot]->in_focus = true;
				current_slot = slots[drag_prev_slot];
				item.clear();
				drag_prev_slot = -1;
				return item;
			}
			else {
				slots[drag_prev_slot]->defocus();
				current_slot = NULL;
			}
		}
		if (!item.empty()) {
			if (item.quantity > 1 && !inpt->pressing[Input::CTRL] && (inpt->pressing[Input::SHIFT] || !inpt->usingMouse() || inpt->touch_locked)) {
				// we use an external menu to let the player pick the desired quantity
				// we will subtract from this stack after they've made their decision
				return item;
			}
			subtract( drag_prev_slot, item.quantity);
		}
		// item will be cleared if item.empty() == true
		return item;
	}
	else {
		current_slot = NULL;
		item.clear();
		return item;
	}
}

void MenuItemStorage::itemReturn(ItemStack stack) {
	add( stack, drag_prev_slot);
	if (drag_prev_slot != -1) {
		slots[drag_prev_slot]->checked = false;
	}
	drag_prev_slot = -1;
}

void MenuItemStorage::highlightMatching(const std::string& type) {
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
