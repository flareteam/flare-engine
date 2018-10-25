/*
Copyright © 2015-2016 Justin Jacobs

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
#include "FontEngine.h"
#include "InputState.h"
#include "MenuNumPicker.h"
#include "MessageEngine.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetInput.h"

#include <limits.h>

MenuNumPicker::MenuNumPicker()
	: Menu()
	, value(0)
	, value_min(0)
	, value_max(INT_MAX)
	, spin_ticks(0)
	, spin_increment(1)
	, spin_delay(settings->max_frames_per_sec/6)
	, confirm_clicked(false)
	, cancel_clicked(false)
{

	button_ok = new WidgetButton(WidgetButton::DEFAULT_FILE);
	button_ok->setLabel(msg->get("OK"));

	button_up = new WidgetButton("images/menus/buttons/up.png");
	button_down = new WidgetButton("images/menus/buttons/down.png");

	button_close = new WidgetButton("images/menus/buttons/button_x.png");

	input_box = new WidgetInput(WidgetInput::DEFAULT_FILE);
	input_box->only_numbers = true;

	label.setText(msg->get("Enter amount:"));
	label.setColor(font->getColor(FontEngine::COLOR_MENU_NORMAL));

	// Load config settings
	FileParser infile;
	// @CLASS MenuNumPicker|Description of menus/num_picker.txt
	if(infile.open("menus/num_picker.txt", FileParser::MOD_FILE, FileParser::ERROR_NORMAL)) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			else if (infile.key == "label_title") {
				// @ATTR label_title|label|Position of the "Enter amount:" text.
				label.setFromLabelInfo(Parse::popLabelInfo(infile.val));
			}
			else if (infile.key == "confirm") {
				// @ATTR confirm|point|Position of the "OK" button.
				Point pos = Parse::toPoint(infile.val);
				button_ok->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else if (infile.key == "increase") {
				// @ATTR increase|point|Position of the button used to increase the value.
				Point pos = Parse::toPoint(infile.val);
				button_up->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else if (infile.key == "decrease") {
				// @ATTR decrease|point|Position of the button used to decrease the value.
				Point pos = Parse::toPoint(infile.val);
				button_down->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else if (infile.key == "close") {
				// @ATTR close|point|Position of the button used to close the number picker window.
				Point pos = Parse::toPoint(infile.val);
				button_close->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else if (infile.key == "input") {
				// @ATTR input|point|Position of the text input box.
				Point pos = Parse::toPoint(infile.val);
				input_box->setBasePos(pos.x, pos.y, Utils::ALIGN_TOPLEFT);
			}
			else
				infile.error("MenuNumPicker: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	setBackground("images/menus/num_picker_bg.png");

	tablist.add(button_up);
	tablist.add(button_down);
	tablist.add(input_box);
	tablist.add(button_ok);
	tablist.add(button_close);

	align();
}

void MenuNumPicker::align() {
	Menu::align();

	button_ok->setPos(window_area.x, window_area.y);
	button_up->setPos(window_area.x, window_area.y);
	button_down->setPos(window_area.x, window_area.y);
	button_close->setPos(window_area.x, window_area.y);

	input_box->setPos(window_area.x, window_area.y);

	label.setPos(window_area.x, window_area.y);

	updateInput();
}

void MenuNumPicker::logic() {
	if (visible) {
		if (!input_box->edit_mode) {
			tablist.logic();
		}

		input_box->logic();

		if (inpt->pressing[Input::CANCEL] && !inpt->lock[Input::CANCEL]) {
			inpt->lock[Input::CANCEL] = true;
			cancel_clicked = true;
		}
		else if (button_close->checkClick()) {
			cancel_clicked = true;
		}
		else if (inpt->pressing[Input::ACCEPT] && !inpt->lock[Input::ACCEPT]) {
			inpt->lock[Input::ACCEPT] = true;
			confirm_clicked = true;
		}
		else if (button_ok->checkClick()) {
			confirm_clicked = true;
		}
		else if (button_up->checkClick()) {
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

		if (confirm_clicked) {
			setValue(Parse::toInt(input_box->getText()));
			updateInput();
		}

		// cancel_clicked is handled in MenuManager; need to stay visible
	}
}

void MenuNumPicker::render() {
	if (visible) {
		// background
		Menu::render();

		label.render();

		button_ok->render();
		button_up->render();
		button_down->render();
		button_close->render();
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
	value = std::min(std::max(val, value_min), value_max);
	updateInput();
}

void MenuNumPicker::increaseValue(int val) {
	setValue(Parse::toInt(input_box->getText()));

	value += val;

	if (value > value_max || value < value_min)
		value = value_max;

	updateInput();
}

void MenuNumPicker::decreaseValue(int val) {
	setValue(Parse::toInt(input_box->getText()));

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
	delete button_up;
	delete button_down;
	delete input_box;
	delete button_close;
}

