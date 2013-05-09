/*
Copyright © 2013 Igor Paliychuk

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


#pragma once
#ifndef WIDGET_SLOT_H
#define WIDGET_SLOT_H

#include "Widget.h"
#include "WidgetTooltip.h"
#include "SharedIncludes.h"

enum CLICK_TYPE {

	NO_CLICK = 0,
	CHECKED = 1,
	ACTIVATED = 2,
};

class WidgetSlot : public Widget {
private:

	SDL_Surface *icons;	// icons surface
	SDL_Surface *slot_selected;
	SDL_Surface *slot_checked;

	int icon_id;		// current slot id
	int amount;			// entries amount in slot
	int max_amount;		// if > 1 always display amount
	int ACTIVATE;

public:
	WidgetSlot(SDL_Surface *_icon, int _icon_id = -1, int _ACTIVATE = ACCEPT);
	~WidgetSlot();

	void activate();
	void deactivate();

	CLICK_TYPE checkClick();
	CLICK_TYPE checkClick(int x, int y);
	void setIcon(int _icon_id);
	void setAmount(int _amount, int _max_amount = 1);
	void render(SDL_Surface *target = NULL);
	void renderSelection(SDL_Surface *target = NULL);

	bool enabled;
	bool checked;
	bool pressed;
};

#endif
