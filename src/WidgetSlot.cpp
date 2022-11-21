/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2013-2016 Justin Jacobs

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
 * class WidgetSlot
 */

#include "CommonIncludes.h"
#include "EngineSettings.h"
#include "FontEngine.h"
#include "IconManager.h"
#include "InputState.h"
#include "RenderDevice.h"
#include "SharedResources.h"
#include "Utils.h"
#include "WidgetSlot.h"

WidgetSlot::WidgetSlot(int _icon_id)
	: Widget()
	, slot_selected(NULL)
	, label_amount_bg(NULL)
	, label_hotkey_bg(NULL)
	, icon_id(_icon_id)
	, overlay_id(NO_ICON)
	, amount(1)
	, max_amount(1)
	, amount_str("")
	, hotkey(-1)
	, activated(false)
	, enabled(true)
	, continuous(false)
	, visible(true)
{
	label_amount.setFromLabelInfo(eset->widgets.slot_quantity_label);
	label_amount.setColor(eset->widgets.slot_quantity_color);
	label_hotkey.setFromLabelInfo(eset->widgets.slot_hotkey_label);
	label_hotkey.setColor(eset->widgets.slot_hotkey_color);

	// in case the hotkey string is long (we only have a fixed set of short keynames), keep the label width to the icon size
	// TODO should this be done for the quantity as well?
	label_hotkey.setMaxWidth(eset->resolutions.icon_size);

	pos.x = pos.y = 0;

	Rect src;
	src.x = src.y = 0;

	std::string selected_filename;

	pos.w = eset->resolutions.icon_size;
	pos.h = eset->resolutions.icon_size;
	src.w = src.h = eset->resolutions.icon_size;

	selected_filename = "images/menus/slot_selected.png";

	Image *graphics;
	graphics = render_device->loadImage(selected_filename, RenderDevice::ERROR_NORMAL);
	if (graphics) {
		slot_selected = graphics->createSprite();
		slot_selected->setClipFromRect(src);
		graphics->unref();
	}
}

void WidgetSlot::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);

	label_amount.setPos(pos.x + icons->text_offset.x, pos.y + icons->text_offset.y);
	label_hotkey.setPos(pos.x + icons->text_offset.x, pos.y + icons->text_offset.y);

	if (label_amount_bg) {
		Rect *r = label_amount.getBounds();
		label_amount_bg->setDest(r->x, r->y);
	}

	if (label_hotkey_bg) {
		Rect *r = label_hotkey.getBounds();
		label_hotkey_bg->setDest(r->x, r->y);
	}
}

void WidgetSlot::activate() {
	activated = true;
}

void WidgetSlot::defocus() {
	in_focus = false;
}

bool WidgetSlot::getNext() {
	return false;
}

bool WidgetSlot::getPrev() {
	return false;
}

WidgetSlot::CLICK_TYPE WidgetSlot::checkClick() {
	return checkClick(inpt->mouse.x,inpt->mouse.y);
}

WidgetSlot::CLICK_TYPE WidgetSlot::checkClick(int x, int y) {
	if (!enabled) {
		return NO_CLICK;
	}

	Point mouse(x,y);
	bool mouse_in_rect = Utils::isWithinRect(pos, mouse);

	if (mouse_in_rect && inpt->pressing[Input::MAIN1] && !inpt->lock[Input::MAIN1]) {
		inpt->lock[Input::MAIN1] = true;
		return DRAG;
	}
	else if (mouse_in_rect && inpt->pressing[Input::MAIN2] && !inpt->lock[Input::MAIN2]) {
		inpt->lock[Input::MAIN2] = true;
		return ACTIVATE;
	}
	else if (in_focus && inpt->pressing[Input::ACCEPT] && !inpt->lock[Input::ACCEPT]) {
		// TODO This probably isn't needed, since Input::ACCEPT is handled by TabList (which calls activate())
		inpt->lock[Input::ACCEPT] = true;
		return DRAG;
	}
	else if (in_focus && inpt->pressing[Input::MENU_ACTIVATE] && !inpt->lock[Input::MENU_ACTIVATE]) {
		inpt->lock[Input::MENU_ACTIVATE] = true;
		return ACTIVATE;
	}
	else if (activated) {
		// activate() was called
		activated = false;
		return DRAG;
	}

	if (continuous) {
		bool continuous_mouse = mouse_in_rect && (inpt->lock[Input::MAIN2] || inpt->touch_locked);
		bool continuous_button = in_focus && inpt->lock[Input::MENU_ACTIVATE];

		if (continuous_mouse || continuous_button) {
			return ACTIVATE;
		}
	}

	return NO_CLICK;
}

int WidgetSlot::getIcon() {
	return icon_id;
}

void WidgetSlot::setIcon(int _icon_id, int _overlay_id) {
	icon_id = _icon_id;
	overlay_id = _overlay_id;
}

void WidgetSlot::setAmount(int _amount, int _max_amount) {
	amount = _amount;
	max_amount = _max_amount;

	amount_str = Utils::abbreviateKilo(amount);

	if ((amount > 1 || max_amount > 1) && !eset->widgets.slot_quantity_label.hidden) {
		label_amount.setPos(pos.x + icons->text_offset.x, pos.y + icons->text_offset.y);
		label_amount.setText(amount_str);
		label_amount.local_frame = local_frame;
		label_amount.local_offset = local_offset;

		Rect* r = label_amount.getBounds();
		if (!label_amount_bg || label_amount_bg->getGraphicsWidth() != r->w || label_amount_bg->getGraphicsHeight() != r->h) {
			if (label_amount_bg) {
				delete label_amount_bg;
				label_amount_bg = NULL;
			}

			if (eset->widgets.slot_quantity_bg_color.a != 0) {
				Image *temp = render_device->createImage(r->w, r->h);
				if (temp) {
					temp->fillWithColor(eset->widgets.slot_quantity_bg_color);
					label_amount_bg = temp->createSprite();
					temp->unref();
				}
			}

			if (label_amount_bg) {
				label_amount_bg->setDest(r->x, r->y);
			}
		}
	}
}

void WidgetSlot::setHotkey(int key) {
	hotkey = key;

	if (hotkey != -1 && !eset->widgets.slot_hotkey_label.hidden) {
		label_hotkey.setPos(pos.x + icons->text_offset.x, pos.y + icons->text_offset.y);
		label_hotkey.setText(inpt->getBindingString(hotkey, InputState::GET_SHORT_STRING));
		label_hotkey.local_frame = local_frame;
		label_hotkey.local_offset = local_offset;

		Rect* r = label_hotkey.getBounds();
		if (!label_hotkey_bg || label_hotkey_bg->getGraphicsWidth() != r->w || label_hotkey_bg->getGraphicsHeight() != r->h) {
			if (label_hotkey_bg) {
				delete label_hotkey_bg;
				label_hotkey_bg = NULL;
			}

			if (eset->widgets.slot_hotkey_bg_color.a != 0) {
				Image *temp = render_device->createImage(r->w, r->h);
				if (temp) {
					temp->fillWithColor(eset->widgets.slot_hotkey_bg_color);
					label_hotkey_bg = temp->createSprite();
					temp->unref();
				}
			}

			if (label_hotkey_bg) {
				label_hotkey_bg->setDest(r->x, r->y);
			}
		}
	}
}

void WidgetSlot::render() {
	if (!visible) return;

	Rect src;

	if (icon_id != -1 && icons) {
		icons->setIcon(icon_id, Point(pos.x, pos.y));
		icons->render();

		if (overlay_id != -1) {
			icons->setIcon(overlay_id, Point(pos.x, pos.y));
			icons->render();
		}

		if (amount > 1 || max_amount > 1) {
			if (label_amount_bg)
				render_device->render(label_amount_bg);
			label_amount.render();
		}
	}
	if (hotkey != -1) {
		// reload the hotkey label if keybindings have changed
		if (inpt->refresh_hotkeys)
			setHotkey(hotkey);

		if (label_hotkey_bg)
			render_device->render(label_hotkey_bg);
		label_hotkey.render();
	}
	renderSelection();
}

/**
 * We can use this function if slot is grayed out to refresh selection frame
 */
void WidgetSlot::renderSelection() {
	if (!visible) return;

	if (in_focus && slot_selected) {
		slot_selected->local_frame = local_frame;
		slot_selected->setOffset(local_offset);
		slot_selected->setDestFromRect(pos);
		render_device->render(slot_selected);
	}
}

WidgetSlot::~WidgetSlot() {
	delete slot_selected;
	delete label_amount_bg;
	delete label_hotkey_bg;
}
