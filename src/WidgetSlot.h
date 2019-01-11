/*
Copyright © 2013 Igor Paliychuk
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2013 Justin Jacobs

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

#ifndef WIDGET_SLOT_H
#define WIDGET_SLOT_H

#include "CommonIncludes.h"
#include "InputState.h"
#include "Widget.h"
#include "WidgetLabel.h"

class WidgetSlot : public Widget {
private:
	Sprite *slot_selected;
	Sprite *slot_checked;
	Sprite *label_bg;

	WidgetLabel label_amount;
	int icon_id;		// current slot id
	int overlay_id;     // icon id for the overlay image
	int amount;			// entries amount in slot
	int max_amount;		// if > 1 always display amount
	std::string amount_str; // formatted display of amount
	int activate_key;

public:
	enum CLICK_TYPE {
		NO_CLICK = 0,
		CHECKED = 1,
		ACTIVATED = 2
	};

	static const int NO_ICON = -1;
	static const int NO_OVERLAY = -1;

	WidgetSlot(int _icon_id, int _activate_key);
	~WidgetSlot();

	void setPos(int offset_x, int offset_y);

	void activate();
	void deactivate();
	void defocus();
	bool getNext();
	bool getPrev();

	CLICK_TYPE checkClick();
	CLICK_TYPE checkClick(int x, int y);
	int getIcon();
	void setIcon(int _icon_id, int _overlay_id);
	void setAmount(int _amount, int _max_amount);
	void render();
	void renderSelection();

	bool enabled;
	bool checked;
	bool pressed;
	bool continuous;	// allow holding key to keep slot activated
};

#endif
