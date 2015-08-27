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

#include "FileParser.h"
#include "MenuNumPicker.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

#include <limits.h>

MenuNumPicker::MenuNumPicker()
	: Menu()
	, value(0)
	, value_min(0)
	, value_max(INT_MAX)
	, spin_ticks(0)
	, spin_increment(1)
	, spin_delay(MAX_FRAMES_PER_SEC/6)
	, spin_rate(1)
{

	button_ok = new WidgetButton();
	button_ok->label = msg->get("OK");

	button_close = new WidgetButton("images/menus/buttons/button_x.png");

	button_up = new WidgetButton("images/menus/buttons/up.png");
	button_down = new WidgetButton("images/menus/buttons/down.png");

	input_box = new WidgetInput();

	// Load config settings
	FileParser infile;
	// @CLASS MenuNumPicker|Description of menus/num_picker.txt
	if(infile.open("menus/num_picker.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else if (infile.key == "close") {
				// @ATTR close|x (integer), y (integer)|Position of the close button.
				Point pos = toPoint(infile.val);
				button_close->setBasePos(pos.x, pos.y);
			}
			else if (infile.key == "label_title") {
				// @ATTR label_title|label|Position of the "Enter amount:" text.
				title = eatLabelInfo(infile.val);
			}
			else if (infile.key == "confirm") {
				// @ATTR confirm|x (integer), y (integer)|Position of the "OK" button.
				Point pos = toPoint(infile.val);
				button_ok->setBasePos(pos.x, pos.y);
			}
			else if (infile.key == "increase") {
				// @ATTR increase|x (integer), y (integer)|Position of the button used to increase the value.
				Point pos = toPoint(infile.val);
				button_up->setBasePos(pos.x, pos.y);
			}
			else if (infile.key == "decrease") {
				// @ATTR decrease|x (integer), y (integer)|Position of the button used to decrease the value.
				Point pos = toPoint(infile.val);
				button_down->setBasePos(pos.x, pos.y);
			}
			else if (infile.key == "input") {
				// @ATTR input|x (integer), y (integer)|Position of the text input box.
				Point pos = toPoint(infile.val);
				input_box->setBasePos(pos.x, pos.y);
			}
			else
				infile.error("MenuNumPicker: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	setBackground("images/menus/num_picker_bg.png");

	tablist.add(button_up);
	tablist.add(button_down);
	tablist.add(button_ok);
	tablist.add(button_close);

	align();
}

void MenuNumPicker::align() {
	Menu::align();

	button_close->setPos(window_area.x, window_area.y);

	button_ok->setPos(window_area.x, window_area.y);
	button_up->setPos(window_area.x, window_area.y);
	button_down->setPos(window_area.x, window_area.y);

	input_box->setPos(window_area.x, window_area.y);

	label.set(window_area.x + title.x, window_area.y + title.y, title.justify, title.valign, msg->get("Enter amount:"), font->getColor("menu_normal"));

	updateInput();
}

void MenuNumPicker::logic() {
	if (visible) {
		tablist.logic();

		if (button_ok->checkClick()) {
			// confirm_clicked = true;
		}
		if (button_close->checkClick()) {
			visible = false;
		}

		if (button_up->checkClick()) {
			increaseValue(1);
		}
		else if (button_down->checkClick()) {
			decreaseValue(1);
		}
		else {
			if (button_up->pressed || button_down->pressed) {
				spin_ticks++;
			}
			else {
				spin_ticks = 0;
				spin_increment = 1;
			}

			if (spin_ticks > 0 && spin_ticks % spin_delay == 0) {
				for (int i=1; i<=6; ++i) {
					if (spin_ticks % (spin_delay*10*i) == 0)
						spin_increment = static_cast<int>(powf(10, static_cast<float>(i)));
				}
				if (button_up->pressed) {
					increaseValue(spin_increment);
				}
				else if (button_down->pressed) {
					decreaseValue(spin_increment);
				}
			}
		}
	}
}

void MenuNumPicker::render() {
	if (visible) {
		// background
		Menu::render();

		label.render();

		button_close->render();

		button_ok->render();
		button_up->render();
		button_down->render();
		input_box->render();
	}
}

void MenuNumPicker::setValueBounds(int low, int high) {
	value_min = low;
	value_max = high;
	value = value_min;
	updateInput();
}

void MenuNumPicker::setValue(int val) {
	value = val;
	clampFloor(value, value_min);
	clampCeil(value, value_max);
	updateInput();
}

void MenuNumPicker::increaseValue(int val) {
	value += val;

	if (value > value_max || value < value_min)
		value = value_max;

	updateInput();
}

void MenuNumPicker::decreaseValue(int val) {
	value -= val;

	if (value > value_max || value < value_min)
		value = value_min;

	updateInput();
}

int MenuNumPicker::getValue() {
	return value;
}

void MenuNumPicker::updateInput() {
	if (!input_box) return;

	std::stringstream ss;
	ss << value;
	input_box->setText(ss.str());
}

MenuNumPicker::~MenuNumPicker() {
	delete button_ok;
	delete button_close;
	delete button_up;
	delete button_down;
	delete input_box;
}

