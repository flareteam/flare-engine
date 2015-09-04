/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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
 * class WidgetButton
 */

#include "CommonIncludes.h"
#include "WidgetSlot.h"
#include "SharedResources.h"
#include "Settings.h"

WidgetSlot::WidgetSlot(int _icon_id, int _ACTIVATE)
	: Widget()
	, slot_selected(NULL)
	, slot_checked(NULL)
	, icon_id(_icon_id)
	, amount(1)
	, max_amount(1)
	, amount_str("")
	, ACTIVATE(_ACTIVATE)
	, enabled(true)
	, checked(false)
	, pressed(false)
	, continuous(false) {
	focusable = true;
	pos.x = pos.y = 0;
	pos.w = ICON_SIZE;
	pos.h = ICON_SIZE;

	Rect src;
	src.x = src.y = 0;
	src.w = src.h = ICON_SIZE;


	Image *graphics;
	graphics = render_device->loadImage("images/menus/slot_selected.png");
	if (graphics) {
		slot_selected = graphics->createSprite();
		slot_selected->setClip(src);
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/slot_checked.png");
	if (graphics) {
		slot_checked = graphics->createSprite();
		slot_checked->setClip(src);
		graphics->unref();
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

CLICK_TYPE WidgetSlot::checkClick() {
	return checkClick(inpt->mouse.x,inpt->mouse.y);
}

CLICK_TYPE WidgetSlot::checkClick(int x, int y) {
	Point mouse(x,y);

	// disabled slots can't be clicked;
	if (!enabled) return NO_CLICK;

	if (continuous && pressed && checked && (inpt->lock[MAIN2] || inpt->lock[ACTIVATE] || inpt->touch_locked)) return ACTIVATED;

	// main button already in use, new click not allowed
	if (inpt->lock[MAIN1]) return NO_CLICK;
	if (inpt->lock[MAIN2]) return NO_CLICK;
	if (inpt->lock[ACTIVATE]) return NO_CLICK;

	if (pressed && !inpt->lock[MAIN1] && !inpt->lock[MAIN2] && !inpt->lock[ACTIVATE]) { // this is a button release
		pressed = false;

		checked = !checked;
		if (checked)
			return CHECKED;
		else
			return ACTIVATED;
	}

	// detect new click
	// use MAIN1 only for selecting
	if (inpt->pressing[MAIN1]) {
		if (isWithin(pos, mouse)) {

			inpt->lock[MAIN1] = true;
			pressed = true;
			checked = false;
		}
	}
	// use MAIN2 only for activating
	if (inpt->pressing[MAIN2]) {
		if (isWithin(pos, mouse)) {

			inpt->lock[MAIN2] = true;
			pressed = true;
			checked = true;
		}
	}

	// handle touch presses for action bar
	if (continuous && inpt->touch_locked) {
		if (isWithin(pos, mouse)) {
			pressed = true;
			checked = true;
			return ACTIVATED;
		}
	}

	return NO_CLICK;

}

void WidgetSlot::setIcon(int _icon_id) {
	icon_id = _icon_id;
}

void WidgetSlot::setAmount(int _amount, int _max_amount) {
	amount = _amount;
	max_amount = _max_amount;

	amount_str = abbreviateKilo(amount);
}

void WidgetSlot::render() {
	Rect src;

	if (icon_id != -1 && icons) {
		int columns = icons->getGraphicsWidth() / ICON_SIZE;
		src.x = (icon_id % columns) * ICON_SIZE;
		src.y = (icon_id / columns) * ICON_SIZE;

		src.w = pos.w;
		src.h = pos.h;

		icons->local_frame = local_frame;
		icons->setOffset(local_offset);
		icons->setClip(src);
		icons->setDest(pos);
		render_device->render(icons);

		if (amount > 1 || max_amount > 1) {
			std::stringstream ss;
			ss << amount_str;
			label_amount.set(pos.x + 2, pos.y + 2, JUSTIFY_LEFT, VALIGN_TOP, ss.str(), font->getColor("widget_normal"));
			label_amount.local_frame = local_frame;
			label_amount.local_offset = local_offset;
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
			slot_checked->setDest(pos);
			render_device->render(slot_checked);
		}
		else if (slot_selected) {
			slot_selected->local_frame = local_frame;
			slot_selected->setOffset(local_offset);
			slot_selected->setDest(pos);
			render_device->render(slot_selected);
		}
	}
}

WidgetSlot::~WidgetSlot() {
	if (slot_selected) delete slot_selected;
	if (slot_checked) delete slot_checked;
}
