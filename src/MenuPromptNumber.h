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

#pragma once
#ifndef MENU_PROMPTNUMBER_H
#define MENU_PROMPTNUMBER_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "WidgetButton.h"
#include "WidgetInput.h"
#include "WidgetLabel.h"

class MenuPromptNumber : public Menu {
protected:
	void alignElements();
	void loadGraphics();
	void increase();
	void decrease();

	WidgetInput *input_box;
	WidgetButton *button_increase;
	WidgetButton *button_decrease;
	WidgetButton *button_confirm;
	WidgetButton *button_close;
	WidgetLabel label;
	TabList	tablist;

	std::string title_text;
	int value;

	int speed;
	int speed_ticker;
	int incrementer;
	bool input_focus;

	Point close_pos;
	LabelInfo title;
	Point confirm_pos;
	Point input_box_pos;
	Point increase_pos;
	Point decrease_pos;

public:
	MenuPromptNumber(const std::string& title_text);
	~MenuPromptNumber();

	int getValue();
	void reset();
	bool inputFocus() { return input_focus; }

	void logic();
	virtual void render();

	bool confirm_clicked;
};

#endif
