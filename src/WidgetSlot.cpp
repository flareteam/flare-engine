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
#include "RenderDevice.h"
#include "SharedResources.h"
#include "WidgetSlot.h"

WidgetSlot::WidgetSlot(int _icon_id, int _ACTIVATE)
	: Widget()
	, slot_selected(NULL)
	, slot_checked(NULL)
	, label_bg(NULL)
	, icon_id(_icon_id)
	, overlay_id(NO_ICON)
	, amount(1)
	, max_amount(1)
	, amount_str("")
	, activate_key(_ACTIVATE)
	, enabled(true)
	, checked(false)
	, pressed(false)
	, continuous(false)
{
	focusable = true;
	label_amount.setFromLabelInfo(eset->widgets.slot_quantity_label);

	pos.x = pos.y = 0;

	Rect src;
	src.x = src.y = 0;

	std::string selected_filename;
	std::string checked_filename;

	pos.w = eset->resolutions.icon_size;
	pos.h = eset->resolutions.icon_size;
	src.w = src.h = eset->resolutions.icon_size;

	selected_filename = "images/menus/slot_selected.png";
	checked_filename = "images/menus/slot_checked.png";

	Image *graphics;
	graphics = render_device->loadImage(selected_filename, RenderDevice::ERROR_NORMAL);
	if (graphics) {
		slot_selected = graphics->createSprite();
		slot_selected->setClipFromRect(src);
		graphics->unref();
	}

	graphics = render_device->loadImage(checked_filename, RenderDevice::ERROR_NORMAL);
	if (graphics) {
		slot_checked = graphics->createSprite();
		slot_checked->setClipFromRect(src);
		graphics->unref();
	}
}

void WidgetSlot::setPos(int offset_x, int offset_y) {
	Widget::setPos(offset_x, offset_y);

	label_amount.setPos(pos.x + icons->text_offset.x, pos.y + icons->text_offset.y);

	if (label_bg) {
		Rect *r = label_amount.getBounds();
		label_bg->setDest(r->x, r->y);
	}
}

void WidgetSlot::activate() {
	pressed = true;
}

void WidgetSlot::deactivate() {
	pressed = false;
	checked = false;
}

void WidgetSlot::defocus() {
	in_focus = false;
	pressed = false;
	checked = false;
}

bool WidgetSlot::getNext() {
	pressed = false;
	checked = false;
	return false;
}

bool WidgetSlot::getPrev() {
	pressed = false;
	checked = false;
	return false;
}

WidgetSlot::CLICK_TYPE WidgetSlot::checkClick() {
	return checkClick(inpt->mouse.x,inpt->mouse.y);
}

WidgetSlot::CLICK_TYPE WidgetSlot::checkClick(int x, int y) {
	// disabled slots can't be clicked;
	if (!enabled) return NO_CLICK;

	Point mouse(x,y);

	if (continuous && pressed && checked && (inpt->lock[Input::MAIN2] || inpt->lock[activate_key] || (inpt->touch_locked && Utils::isWithinRect(pos, mouse))))
		return ACTIVATED;

	// main button already in use, new click not allowed
	if (inpt->lock[Input::MAIN1]) return NO_CLICK;
	if (inpt->lock[Input::MAIN2]) return NO_CLICK;
	if (inpt->lock[activate_key]) return NO_CLICK;

	if (pressed && !inpt->lock[Input::MAIN1] && !inpt->lock[Input::MAIN2] && !inpt->lock[activate_key]) { // this is a button release
		pressed = false;

		checked = !checked;
		if (checked)
			return CHECKED;
		else if (continuous)
			return NO_CLICK;
		else
			return ACTIVATED;
	}

	// detect new click
	// use MAIN1 only for selecting
	if (inpt->pressing[Input::MAIN1]) {
		if (Utils::isWithinRect(pos, mouse)) {

			inpt->lock[Input::MAIN1] = true;
			pressed = true;
			checked = false;
		}
	}
	// use MAIN2 only for activating
	if (inpt->pressing[Input::MAIN2]) {
		if (Utils::isWithinRect(pos, mouse)) {

			inpt->lock[Input::MAIN2] = true;
			pressed = true;
			checked = true;
		}
	}

	// handle touch presses for action bar
	if (continuous && inpt->touch_locked) {
		if (Utils::isWithinRect(pos, mouse)) {
			pressed = true;
			checked = true;
			return ACTIVATED;
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

	if (amount > 1 || max_amount > 1) {
		label_amount.setPos(pos.x + icons->text_offset.x, pos.y + icons->text_offset.y);
		label_amount.setText(amount_str);
		label_amount.local_frame = local_frame;
		label_amount.local_offset = local_offset;

		Rect* r = label_amount.getBounds();
		if (!label_bg || label_bg->getGraphicsWidth() != r->w || label_bg->getGraphicsHeight() != r->h) {
			if (label_bg) {
				delete label_bg;
				label_bg = NULL;
			}

			if (eset->widgets.slot_quantity_bg_color.a != 0) {
				Image *temp = render_device->createImage(r->w, r->h);
				if (temp) {
					temp->fillWithColor(eset->widgets.slot_quantity_bg_color);
					label_bg = temp->createSprite();
					temp->unref();
				}
			}

			if (label_bg) {
				label_bg->setDest(r->x, r->y);
			}
		}
	}
}

void WidgetSlot::render() {
	Rect src;

	if (icon_id != -1 && icons) {
		icons->setIcon(icon_id, Point(pos.x, pos.y));
		icons->render();

		if (overlay_id != -1) {
			icons->setIcon(overlay_id, Point(pos.x, pos.y));
			icons->render();
		}

		if (amount > 1 || max_amount > 1) {
			render_device->render(label_bg);
			label_amount.render();
		}
	}
	renderSelection();
}

/**
 * We can use this function if slot is grayed out to refresh selection frame
 */
void WidgetSlot::renderSelection() {
	if (in_focus) {
		if (slot_checked && checked) {
			slot_checked->local_frame = local_frame;
			slot_checked->setOffset(local_offset);
			slot_checked->setDestFromRect(pos);
			render_device->render(slot_checked);
		}
		else if (slot_selected) {
			slot_selected->local_frame = local_frame;
			slot_selected->setOffset(local_offset);
			slot_selected->setDestFromRect(pos);
			render_device->render(slot_selected);
		}
	}
}

WidgetSlot::~WidgetSlot() {
	delete slot_selected;
	delete slot_checked;
	delete label_bg;
}
