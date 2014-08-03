/*
Copyright © 2014 Justin Jacobs

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
 * class MenuPromptNumber
 */

#include "FileParser.h"
#include "MenuPromptNumber.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

#include <limits>

MenuPromptNumber::MenuPromptNumber(const std::string& _title_text)
	: Menu()
	, title_text(_title_text)
	, value(0)
	, speed(MAX_FRAMES_PER_SEC/10)
	, speed_ticker(speed)
	, incrementer(1)
	, input_focus(false)
	, confirm_clicked(false) {

	// Load config settings
	FileParser infile;
	// @CLASS MenuPromptNumber|Description of menus/prompt_list.txt
	if(infile.open("menus/promptnumber.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|x (integer), y (integer)|Position of the close button.
			if(infile.key == "close") close_pos = toPoint(infile.val);
			// @ATTR label_title|label|Position of the menu title.
			else if(infile.key == "label_title") title = eatLabelInfo(infile.val);
			// @ATTR confirm|x (integer), y (integer)|Position of the "OK" button
			else if (infile.key == "confirm") confirm_pos = toPoint(infile.val);
			// @ATTR input_box|x (integer), y (integer)|Position of the text box that displays the number.
			else if (infile.key == "input_box") input_box_pos = toPoint(infile.val);
			// @ATTR increase|x (integer), y (integer)|Position of the button that increases the number.
			else if (infile.key == "increase") increase_pos = toPoint(infile.val);
			// @ATTR decrease|x (integer), y (integer)|Position of the button that decreases the number.
			else if (infile.key == "decrease") decrease_pos = toPoint(infile.val);
		}
		infile.close();
	}

	input_box = new WidgetInput();
	input_box->setText("0");

	button_increase = new WidgetButton("images/menus/buttons/up.png");
	button_decrease = new WidgetButton("images/menus/buttons/down.png");

	button_confirm = new WidgetButton("images/menus/buttons/button_default.png");
	button_confirm->label = msg->get("OK");

	button_close = new WidgetButton("images/menus/buttons/button_x.png");

	setBackground("images/menus/prompt_number_bg.png");

	tablist.add(button_increase);
	tablist.add(button_decrease);
	tablist.add(button_confirm);
	tablist.add(button_close);

	align();
	alignElements();
}

void MenuPromptNumber::alignElements() {
	input_box->setPosition(window_area.x + input_box_pos.x, window_area.y + input_box_pos.y);

	button_increase->pos.x = window_area.x + increase_pos.x;
	button_increase->pos.y = window_area.y + increase_pos.y;

	button_decrease->pos.x = window_area.x + decrease_pos.x;
	button_decrease->pos.y = window_area.y + decrease_pos.y;

	button_confirm->pos.x = window_area.x + confirm_pos.x;
	button_confirm->pos.y = window_area.y + confirm_pos.y;
	button_confirm->refresh();

	button_close->pos.x = window_area.x + close_pos.x;
	button_close->pos.y = window_area.y + close_pos.y;

	label.set(window_area.x + title.x, window_area.y + title.y, title.justify, title.valign, msg->get(title_text), font->getColor("menu_normal"), title.font_style);
}

void MenuPromptNumber::increase() {
	value = toInt(input_box->getText());
	if (value >= std::numeric_limits<int>::max()-incrementer) {
		value = std::numeric_limits<int>::max();
	}
	else {
		value += incrementer;
	}
	input_box->setText(toString(typeid(int), &value));
}

void MenuPromptNumber::decrease() {
	value = toInt(input_box->getText());
	value -= incrementer;
	if (value < 0) {
		value = 0;
	}
	input_box->setText(toString(typeid(int), &value));
}

int MenuPromptNumber::getValue() {
	return value;
}

void MenuPromptNumber::reset() {
	value = 0;
	input_box->setText("0");
	speed_ticker = speed;
	incrementer = 1;
}

void MenuPromptNumber::logic() {
	if (visible) {
		if (NO_MOUSE) {
			tablist.logic();
		}

		input_box->logic();
		input_focus = input_box->inFocus;

		button_increase->checkClick();
		button_decrease->checkClick();

		if (button_increase->pressed || button_decrease->pressed) {
			if (speed_ticker == speed) {
				if (button_increase->pressed) increase();
				else if (button_decrease->pressed) decrease();

				if (incrementer < 5 && value % 5 == 0) incrementer = 5;
				else if (incrementer < 10 && value % 10 == 0) incrementer = 10;
				else if (incrementer < 100 && value % 100 == 0) incrementer = 100;
			}
			speed_ticker--;
			if (speed_ticker == 0) {
				speed_ticker = speed;

			}
		}
		else {
			speed_ticker = speed;
			incrementer = 1;
		}

		if (button_confirm->checkClick()) {
			confirm_clicked = true;
			value = toInt(input_box->getText());
		}
		if (button_close->checkClick()) {
			visible = false;
		}
	}
	else {
		input_focus = false;
		confirm_clicked = false;
	}
}

void MenuPromptNumber::render() {
	if (visible) {
		// background
		Menu::render();

		label.render();

		input_box->render();

		button_increase->render();
		button_decrease->render();

		button_confirm->render();
		button_close->render();
	}
}

MenuPromptNumber::~MenuPromptNumber() {
	delete input_box;
	delete button_increase;
	delete button_decrease;
	delete button_confirm;
	delete button_close;
}

