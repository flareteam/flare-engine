/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller

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
#include "SDL_gfxBlitFunc.h"
#include "Settings.h"

using namespace std;

WidgetSlot::WidgetSlot(SDL_Surface *_icons, int _icon_id, int _ACTIVATE)
	: Widget()
	, icons(_icons)
	, icon_id(_icon_id)
	, amount(1)
	, max_amount(1)
	, ACTIVATE(_ACTIVATE)
	, enabled(true)
	, checked(false)
	, pressed(false) {
	focusable = true;
	pos.x = pos.y = 0;
	pos.w = ICON_SIZE;
	pos.h = ICON_SIZE;

	slot_selected = loadGraphicSurface("images/menus/slot_selected.png");
	slot_checked = loadGraphicSurface("images/menus/slot_checked.png");
}

void WidgetSlot::activate() {
	pressed = true;
}

void WidgetSlot::deactivate() {
	pressed = false;
	checked = false;
}

CLICK_TYPE WidgetSlot::checkClick() {
	return checkClick(inpt->mouse.x,inpt->mouse.y);
}

CLICK_TYPE WidgetSlot::checkClick(int x, int y) {
	Point mouse(x,y);

	// disabled slots can't be clicked;
	if (!enabled) return NO_CLICK;

	if (pressed && checked && (inpt->lock[MAIN2] || inpt->lock[ACTIVATE])) return ACTIVATED;

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
	return NO_CLICK;

}

void WidgetSlot::setIcon(int _icon_id) {
	icon_id = _icon_id;
}

void WidgetSlot::setAmount(int _amount, int _max_amount) {
	amount = _amount;
	max_amount = _max_amount;
}

void WidgetSlot::render(SDL_Surface *target) {
	if (target == NULL) {
		target = screen;
	}
	SDL_Rect src;

	if (icon_id != -1 && icons != NULL) {
		int columns = icons->w / ICON_SIZE;
		src.x = (icon_id % columns) * ICON_SIZE;
		src.y = (icon_id / columns) * ICON_SIZE;

		src.w = pos.w;
		src.h = pos.h;

		if (render_to_alpha)
			SDL_gfxBlitRGBA(icons, &src, target, &pos);
		else
			SDL_BlitSurface(icons, &src, target, &pos);

		if (amount > 1 || max_amount > 1) {
			stringstream ss;
			ss << amount;

			WidgetLabel label;
			label.set(pos.x + 2, pos.y + 2, JUSTIFY_LEFT, VALIGN_TOP, ss.str(), font->getColor("item_normal"));
			label.render();
		}
	}
	renderSelection(target);
}

/**
 * We can use this function if slot is grayed out to refresh selection frame
 */
void WidgetSlot::renderSelection(SDL_Surface *target) {
	if (target == NULL) {
		target = screen;
	}

	if (in_focus) {
		SDL_Rect src;
		src.x = src.y = 0;
		src.w = src.h = ICON_SIZE;

		if (render_to_alpha) {
			if (checked)
				SDL_gfxBlitRGBA(slot_checked, &src, target, &pos);
			else
				SDL_gfxBlitRGBA(slot_selected, &src, target, &pos);
		}
		else {
			if (checked)
				SDL_BlitSurface(slot_checked, &src, target, &pos);
			else
				SDL_BlitSurface(slot_selected, &src, target, &pos);
		}
	}
}

WidgetSlot::~WidgetSlot() {
	SDL_FreeSurface(slot_selected);
	SDL_FreeSurface(slot_checked);
}
