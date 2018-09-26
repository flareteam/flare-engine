/*
Copyright © 2015 Justin Jacobs

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
 * class MenuNumPicker
 */

#ifndef MENU_NUMPICKER_H
#define MENU_NUMPICKER_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "WidgetLabel.h"

class WidgetButton;
class WidgetInput;

class MenuNumPicker : public Menu {
protected:
	void loadGraphics();
	void updateInput();

	WidgetButton *button_ok;
	WidgetButton *button_up;
	WidgetButton *button_down;
	WidgetButton *button_close;
	WidgetInput *input_box;
	WidgetLabel label;

	int value;
	int value_min;
	int value_max;

	int spin_ticks;
	int spin_increment;
	int spin_delay;

public:
	MenuNumPicker();
	~MenuNumPicker();
	void align();

	void logic();
	virtual void render();

	void setValueBounds(int low, int high);
	void setValue(int val);
	void increaseValue(int val);
	void decreaseValue(int val);
	int getValue();

	bool confirm_clicked;
	bool cancel_clicked;
};

#endif
